#ifndef __CSC_MATRIX_H__
#define __CSC_MATRIX_H__

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include <common.h>
#include <vec.h>

#include <iostream>

#ifdef DEBUG
#include <logger.h>
#define LOG(...) log(__VA_ARGS__)
#else
#define LOG(...)
#endif

#ifdef PARALLEL
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif

namespace csc {

template<typename T = float>
    requires numeric<T>
class csc_mat {
private:
    struct element {
        T           m_value = 0;
        std::size_t m_row   = 0;

        constexpr element() = default;

        constexpr element(T value, std::size_t row)
        : m_value(value)
        , m_row(row) {}
    };

private:
    // Size of the matrix.
    std::size_t              m_rows;
    std::size_t              m_cols;
    // Stores only the non-zero elements of the matrix.
    std::vector<element>     m_values;
    // m_col_count[i] = the number of non-zero elements in the columns 0..(i - 1).
    std::vector<std::size_t> m_col_count;

public:
    constexpr csc_mat(
        const std::ranges::range auto &values,
        const std::ranges::range auto &idxs,
        const std::ranges::range auto &col_count,
        const std::size_t rows,
        const std::size_t cols)
    : m_rows(rows)
    , m_cols(cols)
    , m_col_count(cols + 1, 0)
    {
        using values_t    = std::ranges::range_value_t<decltype(values)>;
        using idxs_t      = std::ranges::range_value_t<decltype(idxs)>;
        using col_count_t = std::ranges::range_value_t<decltype(col_count)>;

        static_assert(std::convertible_to<values_t, T>);
        static_assert(std::convertible_to<idxs_t, std::size_t>);
        static_assert(std::integral<col_count_t>);

        const std::size_t size = values.size();
        m_values.reserve(size);

        std::size_t vi = 0; // values index
        std::size_t ci = 0; // column index

        for (std::size_t i = 0; i < size; ++i) {
            while (ci <= cols && std::cmp_greater_equal(i, col_count[ci])) {
                m_col_count[ci++] = vi;
            }

            if (values[i] == 0) {
                continue;
            }

            m_values.emplace_back(element(values[i], idxs[i]));
            ++vi;
        }

        while (ci <= cols) {
            m_col_count[ci++] = vi;
        }

        m_values.resize(vi);

        for (std::size_t i = 0; i < cols; ++i) {
            std::sort(
                std::begin(m_values) + m_col_count[i],
                std::begin(m_values) + m_col_count[i + 1],
                [](const element &lhs, const element &rhs) {
                    return lhs.m_row < rhs.m_row;
                }
            );
        }
    }

    template<typename U>
    constexpr vec<multiply_t<T, U>> operator*(const vec<U> &vector) const {
        using result_type = multiply_t<T, U>;

        if (vector.size() != m_cols) {
            throw std::out_of_range("Invalid size of the vector.");
        }
        
        vec<result_type> result(0, m_rows);
        std::size_t vi = 0; // values index
        std::size_t ci = 0; // column index

        while (ci < m_cols && vi < m_values.size()) {
            const auto limit = m_col_count[ci + 1];
            const auto vv = vector[ci++]; // ci-th value of the vector v

            while (vi < limit) {
                const auto ri = m_values[vi].m_row; // row index
                // DO NOT change the order of multiplication here.
                // Note `multiply_t<T, U>` corresponds to T * U, not U * T.
                result[ri] += m_values[vi].m_value * vv;
                ++vi;
            }
        }

        return result;
    }

    constexpr bool is_upper_triangular() const {
        for (std::size_t i = 0; i < m_cols; ++i) {
            if (m_col_count[i] < m_col_count[i + 1]) {
                const auto &elem = m_values[m_col_count[i + 1] - 1];
                if (elem.m_row > i) {
                    return false;
                }
            }
        }

        return true;
    }

    template<typename U>
    constexpr vec<multiply_t<T, U>> solve_upper_triangular(const vec<U> &vector) const {
        using result_type = multiply_t<T, U>;

        if (m_cols != vector.size()) {
            throw std::out_of_range("Vector is of invalid size.");
        }

        // The matrix is not upper-triangular.
        ASSERT(is_upper_triangular());

        const auto size = vector.size();
        auto revv = m_values    | std::views::reverse; // Reversed values
        auto revc = m_col_count | std::views::reverse; // Reversed columns
        vec<result_type> result(vector);

        std::size_t vi = 0;
        std::size_t ci = 0;

        while (vi < size) {
            const std::size_t limit = revc[ci] - revc[ci + 1];
            const std::size_t idx = m_rows - ci - 1;

            // The matrix is not invertible.
            ASSERT(revv[vi].m_row   == ci);
            ASSERT(revv[vi].m_value !=  0);
            
            result[idx] /= revv[vi].m_value;

            for (std::size_t offset = 1; offset < limit; ++offset) {
                const auto &value = revv[vi + offset];
                result[value.m_row] -= value.m_value * result[idx];
            }

            vi += limit;
            ++ci;
        }

        return result;
    }

    T find_max_eigenvalue(const T threshhold = 1e-6, const bool use_relative_error = true) const {
        // We define eigenvalues only for square matrices.
        ASSERT(m_cols == m_rows);

        constexpr std::size_t MAX_ATTEMPTS_COUNT         = 10;
        constexpr std::size_t MAX_ITERATION_COUNT        = 50;
        constexpr std::size_t MAX_SINGLE_ITERATION_COUNT = 20;

        for (std::size_t i = 0; i < MAX_ATTEMPTS_COUNT; ++i) {
            // Iteration vector: choose at random
            vec<T> it_vec = vec<T>::random_vec(m_cols);
            
            // Perform the power iteration method at most
            //   MAX_ITERATION_COUNT * MAX_SINGLE_ITERATION_COUNT
            // times. If an eigenvalue has been found, return it.
            // Otherwise choose a new random vector.
            for (std::size_t j = 0; j < MAX_ITERATION_COUNT; ++j) {
                for (std::size_t u = 0; u < MAX_SINGLE_ITERATION_COUNT; ++u) {
                    it_vec = this[0] * it_vec;
                    it_vec.normalize();
                }

                // The vector of the next iteration
                const auto new_it_vec = this[0] * it_vec;

                const T numerator = std::inner_product(
                    std::begin(it_vec),
                    std::end(it_vec),
                    std::begin(new_it_vec),
                    T(0)
                );

                const T denominator = std::inner_product(
                    std::begin(it_vec),
                    std::end(it_vec),
                    std::begin(it_vec),
                    T(0)
                );

                if (denominator == 0) {
                    it_vec = new_it_vec;
                    it_vec.normalize();
                    continue;
                }

                // Rayleigh's quotient 
                const T eigenvalue = std::abs(numerator) / denominator;
                
                // There's no meaning in continuing with the current vector.
                if (eigenvalue == 0) {
                    break;
                }

                const vec<T> error_vec = new_it_vec - eigenvalue * it_vec;

                const T error = [&]() {
                    return use_relative_error
                            ? error_vec.norm() / (eigenvalue * it_vec.norm())
                            : error_vec.norm();
                }();

                if (error <= threshhold) {
                    return eigenvalue;
                }

                // new_it_vec is the value of the next iteration,
                // so there's no need to compute it once again.
                it_vec = new_it_vec;
                it_vec.normalize();
            }
        }

        throw std::runtime_error("No eigenvalue satisfying the constraint has been found.");
    }
};

} // namespace csc

#endif // __CSC_MATRIX_H__

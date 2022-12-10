#ifndef __CSC_MATRIX_H__
#define __CSC_MATRIX_H__

#include <algorithm>
#include <concepts>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <common.h>
#include <vec.h>

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

template<typename T, std::size_t R, std::size_t C>
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
    // Stores only the non-zero elements of the matrix.
    std::vector<element>     m_values;
    // m_col_count[i] = the number of non-zero elements in the columns 0..(i - 1).
    std::vector<std::size_t> m_col_count;

public:
    // The compiler cannot perform implicit conversion from C-style arrays to std::span
    // when there are multiple arguments provided. We need to do that by hand.
    template<typename U, std::size_t N, typename I, typename J>
    csc_mat(const U (&data)[N], I &&indices, J &&col_count)
    : csc_mat(std::span(data), std::forward<I>(indices), std::forward<J>(col_count)) {}

    template<typename U, std::size_t N, typename I, std::size_t M, typename J>
    csc_mat(const std::span<U, N> data, const I (&indices)[M], J &&col_count)
    : csc_mat(data, std::span(indices), std::forward<J>(col_count)) {}

    template<typename U, std::size_t N, typename I, std::size_t M, typename J, std::size_t K>
    csc_mat(const std::span<U, N> data, const std::span<I, M> indices, const J (&col_count)[K])
    : csc_mat(data, indices, std::span(col_count)) {}

    template<typename U, std::size_t N, typename I, std::size_t M, typename J, std::size_t K>
        requires std::convertible_to<U, T> &&
                 std::convertible_to<I, std::size_t> &&
                 std::convertible_to<J, std::size_t>
    constexpr csc_mat(
        const std::span<const U, N> values,
        const std::span<const I, M> rows,
        const std::span<const J, K> col_count)
    : m_col_count(C + 1, 0)
    {
        const std::size_t size = values.size();
        m_values.reserve(size);

        std::size_t vi = 0; // values index
        std::size_t ci = 0; // column index

        for (std::size_t i = 0; i < size; ++i) {
            if (values[i] == 0) {
                continue;
            }

            m_values.emplace_back(element(values[i], rows[i]));
            ++vi;

            while (ci + 1 < col_count.size() && std::cmp_greater_equal(i + 1, col_count[ci])) {
                m_col_count[++ci] = vi;
            }
        }

        m_values.resize(vi);

        for (std::size_t i = 0; i < C; ++i) {
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
        if (vector.size() != C) {
            throw std::out_of_range("Invalid size of the vector.");
        }
        
        vec<result_type> result(0, R);
        std::size_t vi = 0; // values index
        std::size_t ci = 0; // column index

        while (ci < C && vi < m_values.size()) {
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
        for (std::size_t i = 0; i < C; ++i) {
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

        if (C != vector.size()) {
            throw std::out_of_range("Vector is of invalid size.");
        }
        ASSERT("The matrix is not upper-triangular.", is_upper_triangular());

        const auto size = vector.size();
        auto revv = m_values    | std::views::reverse; // Reversed values
        auto revc = m_col_count | std::views::reverse; // Reversed columns
        vec<result_type> result(vector);

        std::size_t vi = 0;
        std::size_t ci = 0;

        while (vi < size) {
            const std::size_t limit = revc[ci] - revc[ci + 1];
            const std::size_t idx = R - ci - 1;

            ASSERT("The matrix is not invertible.", revv[vi].m_row   == ci);
            ASSERT("The matrix is not invertible.", revv[vi].m_value !=  0);
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

    T find_max_eigenvalue(const T threshhold = 1e-8) const {
        static_assert(C == R, "We define eigenvalues only for square matrices.");

        constexpr std::size_t MAX_ATTEMPTS_COUNT         = 5;
        constexpr std::size_t MAX_ITERATION_COUNT        = 50;
        constexpr std::size_t MAX_SINGLE_ITERATION_COUNT = 20;

        for (std::size_t i = 0; i < MAX_ATTEMPTS_COUNT; ++i) {
            // Iteration vector: choose at random
            vec<T> it_vec = vec<T>::random_vec(C, 0.0L, 1.0L);
            
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

                // Approximate the eigenvalue. Choose the biggest value
                // out of the fractions of non-zero elements of it_vec
                // and their non-zero counterparts in new_it_vec.
                const auto eigenvalue = std::transform_reduce(
                    PAR
                    std::begin(new_it_vec),
                    std::end(new_it_vec),
                    std::begin(it_vec),
                    static_cast<T>(0),
                    [](const auto lhs, const auto rhs) {
                        return std::max(lhs, rhs);
                    },
                    [](const auto lhs, const auto rhs) {
                        return (lhs != 0 && rhs != 0)
                                ? std::abs(lhs / rhs)
                                : static_cast<T>(0);
                    }
                );
                
                // There's no meaning in continuing with the current vector.
                if (eigenvalue == 0) {
                    break;
                }

                const auto total_error = std::transform_reduce(
                    PAR
                    std::begin(new_it_vec),
                    std::end(new_it_vec),
                    std::begin(it_vec),
                    0.0L,
                    std::plus{},
                    [eigenvalue](const auto lhs, const auto rhs) {
                        return std::abs(std::abs(lhs) / eigenvalue - std::abs(rhs));
                    }
                );

                if (total_error <= C * threshhold) {
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

#ifndef __CSC_VEC_H__
#define __CSC_VEC_H__

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <numeric>
#include <random>
#include <span>
#include <type_traits>
#include <valarray>

#include <common.h>

#ifdef DEBUG
#include <iostream>
#endif

#ifdef PARALLEL
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif

namespace csc {

template<typename T>
    requires numeric<T>
class vec : public std::valarray<T> {
private:
    using super = std::valarray<T>;

public:
    template<typename... Args>
    constexpr vec(Args &&...args)
    : super(std::forward<Args>(args)...) {}

    template<std::size_t N>
    constexpr vec(std::span<T, N> init_values)
    : super(init_values.data(), init_values.size()) {}

    static inline vec<T> random_vec(std::size_t size, T interval_left = T(0), T interval_right = T(1)) {
        vec<T> result(size);

        std::mt19937 int_gen(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> url(interval_left, interval_right);

        for (auto &value : result) {
            value = url(int_gen);
        }

        return result;
    }

    constexpr T norm() const {
        return std::sqrt(std::transform_reduce(
            PAR
            std::begin(*this),
            std::end(*this),
            static_cast<T>(0),
            std::plus{},
            [](T value) { return value * value; }
        ));
    }

    constexpr T diff_norm(const vec<T> &other) const {
        return std::sqrt(std::transform_reduce(
            PAR
            std::begin(*this),
            std::end(*this),
            std::begin(other),
            static_cast<T>(0),
            std::plus{},
            [](const auto lhs, const auto rhs) {
                const auto result = lhs - rhs;
                return result * result;
            }
        ));
    }

    constexpr T diff_norm(const vec<T> &other, const T scalar) const {
        return std::sqrt(std::transform_reduce(
            PAR
            std::begin(*this),
            std::end(*this),
            std::begin(other),
            static_cast<T>(0),
            std::plus{},
            [scalar](const auto lhs, const auto rhs) {
                const auto result = lhs - scalar * rhs;
                return result * result;
            }
        ));
    }

    constexpr void normalize() {
        *this /= norm();
    }
};

#ifdef DEBUG
template<typename T>
std::ostream &operator<<(std::ostream &stream, const vec<T> &vector) {
    const std::size_t size = vector.size();
    if (size == 0) {
        return stream;
    }

    stream << '[';
    for (std::size_t i = 0; i < size - 1; ++i) {
        stream << vector[i] << ", ";
    }
    stream << vector[size - 1] << ']';
    return stream;
}
#endif

} // namespace csc

#endif // __CSC_VEC_H__

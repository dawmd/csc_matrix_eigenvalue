#ifndef __CSC_COMMON_H__
#define __CSC_COMMON_H__

#include <bits/utility.h>
#include <cassert>
#include <concepts>
#include <type_traits>
#include <utility>      // std::declval

#ifdef DEBUG
#include <cassert>
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(...)
#endif

namespace csc {

template<typename T>
concept numeric =
    std::integral<std::decay_t<T>> ||
    std::floating_point<std::decay_t<T>>;

template<typename T, typename U>
concept addible = requires (T t, U u) {
    { t + u };
};

template<typename T, typename U>
using add_t = decltype(std::declval<T&>() + std::declval<U&>());

template<typename T, typename U>
concept subtractable = requires (T t, U u) {
    { t - u };
};

template<typename T, typename U>
using subtract_t = decltype(std::declval<T&>() - std::declval<U&>());

template<typename T, typename U>
concept multipliable = requires (T t, U u) {
    { t * u };
};

template<typename T, typename U>
using multiply_t = decltype(std::declval<T&>() * std::declval<U&>());

// Choose T if Condition == True. Choose U otherwise.
template<bool Condition, typename T, typename U>
struct choose_type;

template<typename T, typename U>
struct choose_type<true, T, U> {
    using type = T;
};

template<typename T, typename U>
struct choose_type<false, T, U> {
    using type = U;
};

template<bool Condition, typename T, typename U>
using choose_type_t = typename choose_type<Condition, T, U>::type;

} // namespace csc

#endif // __CSC_COMMON_H__

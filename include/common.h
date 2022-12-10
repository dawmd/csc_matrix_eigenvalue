#ifndef __CSC_COMMON_H__
#define __CSC_COMMON_H__

#include <bits/utility.h>
#include <cassert>
#include <concepts>
#include <type_traits>
#include <utility>      // std::declval

#ifdef DEBUG
#include <cassert>
#define ASSERT(...) assert((__VA_ARGS__))
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

template<std::size_t Count, typename T>
class sequence {
public:
    struct iterator {
        using value_type = T;

        const T     m_value;
        std::size_t m_counter = 0;

        constexpr iterator(T value) : m_value(value) {}

        constexpr T &operator*() const noexcept { return m_value; }
        constexpr T *operator->() const noexcept { return &m_value; }

        constexpr iterator &operator++() noexcept {
            ++m_counter;
            return *this;
        }

        constexpr iterator operator++(int) noexcept {
            iterator result { .m_counter = this->m_counter };
            ++m_counter;
            return result;
        }

        constexpr iterator &operator--() noexcept {
            --m_counter;
            return *this;
        }

        constexpr iterator operator--(int) noexcept {
            iterator result { .m_counter = this->m_counter };
            --m_counter;
            return result;
        }

        constexpr bool operator==(const iterator &other) const noexcept {
            return m_counter == other.m_counter;
        }

        constexpr bool operator!=(const iterator &other) const noexcept {
            return m_counter != other.m_counter;
        }
    };

private:
    T m_value;

public:
    constexpr sequence(const T value) : m_value(value) {}

    constexpr auto begin() const {
        return iterator(m_value);
    }

    constexpr auto end() const {
        iterator it(m_value);
        it.m_counter = Count;
        return it;
    }
};

} // namespace csc

#endif // __CSC_COMMON_H__

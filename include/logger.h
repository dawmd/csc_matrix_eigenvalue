#ifndef __CSC_LOGGER_H__
#define __CSC_LOGGER_H__

#include <cstdlib>  // std::exit, EXIT_FAILURE
#include <iostream> // ostreams

// Clang still doesn't support std::source_location
#if __cplusplus >= 202002L && not defined(__clang__)

#include <source_location>

namespace csc {

template<typename T, typename Stream = std::ostream>
concept printable = requires(Stream s, T t) {
    { s << t } -> std::same_as<std::decay_t<Stream>&>;
};

namespace detail {

inline std::ostream &get_stderr_stream() {
    return std::cerr;
}

inline std::ostream &get_stdout_stream() {
    return std::cout;
}

template<typename... Ts>
    requires (printable<Ts> && ...)
inline void log(std::ostream &stream, const Ts &...ts) {
    ([&](const auto &msg) {
        stream << msg;
    }(ts), ...);
    stream << "\n";
}

} // namespace detail

// Log a message
template<typename... Ts>
    requires (printable<Ts> && ...)
inline void log(const Ts &...ts) {
    detail::log(detail::get_stdout_stream(), ts...);
}

// Log an error message
template<typename... Ts>
    requires (printable<Ts> && ...)
inline void elog(const Ts &...ts) {
    detail::log(detail::get_stderr_stream(), ts...);
}

// Exit the program with a message
template<typename... Ts>
    requires (printable<Ts> && ...)
inline void fail(const Ts &...ts) {
    elog(ts...);
    std::exit(EXIT_FAILURE);
}

namespace detail {

inline void log_location(std::ostream &stream, const std::source_location &location) {
    stream << "[" << location.file_name() << ", " << location.function_name() << ":" << location.line() << "]: ";
}

template<typename... Ts>
    requires (printable<Ts> && ...)
inline void flog(const std::source_location &location, const Ts &...ts) {
    decltype(auto) stream = get_stdout_stream();
    log_location(stream, location);
    log(stream, ts...);
}

template<typename... Ts>
    requires (printable<Ts> && ...)
inline void eflog(const std::source_location &location, const Ts &...ts) {
    decltype(auto) stream = get_stderr_stream();
    log_location(stream, location);
    log(stream, ts...);
}

template<typename... Ts>
    requires (printable<Ts> && ...)
inline void ffail(const std::source_location &location, const Ts &...ts) {
    eflog(location, ts...);
    std::exit(EXIT_FAILURE);
}

} // namespace detail
} // namespace csc

// Log with the location
#define  flog(...)  detail::flog(std::source_location::current(), __VA_ARGS__)
// Log an error with the location
#define eflog(...) detail::eflog(std::source_location::current(), __VA_ARGS__)
// Exit the program with the location
#define ffail(...) detail::ffail(std::source_location::current(), __VA_ARGS__)

template<typename T, std::size_t N>
    requires (!std::same_as<char,          std::decay_t<T>> &&
              !std::same_as<unsigned char, std::decay_t<T>>)
std::ostream &operator<<(std::ostream &stream, const T (&array)[N]) {
    stream << '[';
    for (std::size_t i = 0; i < N - 1; ++i) {
        stream << array[i] << ", ";
    }
    stream << array[N - 1] << ']';
    return stream;
}

#else

#include <type_traits>

namespace csc {

namespace detail {

// SFINAE version
template<typename T>
struct is_ostream_defined_aux {
    template<typename U>
    static auto test(U*) -> decltype(std::declval<std::ostream>() << std::declval<U>());
    template<typename>
    static auto test(...) -> std::false_type;

    using type = typename std::is_same<std::ostream, std::decay_t<decltype(test<T>(0))>>::type;
};

template<typename T>
struct is_ostream_defined : is_ostream_defined_aux<T>::type {};

template<typename... Ts>
inline void log(std::ostream &stream, const Ts &...ts) {
    static_assert((is_ostream_defined<Ts>::value && ...),
        "Operator << is not defined for one of the types passed to the function.");

    ([&](const auto &msg) {
        stream << msg;
    }(ts), ...);
    stream << "\n";
}

inline std::ostream &get_stderr_stream() {
    return std::cerr;
}

inline std::ostream &get_stdout_stream() {
    return std::cout;
}

} // namespace detail

// Log a message
template<typename... Ts>
inline void log(const Ts &...ts) {
    detail::log(detail::get_stdout_stream(), ts...);
}

// Log an error message
template<typename... Ts>
inline void elog(const Ts &...ts) {
    detail::log(detail::get_stderr_stream(), ts...);
}

// Exit the program with a message
template<typename... Ts>
inline void fail(const Ts &...ts) {
    elog(ts...);
    std::exit(EXIT_FAILURE);
}

} // namespace csc

// Log with the location
#define  flog(...)  log("[", __FILE__, ", ", __func__, ':', __LINE__, "]: ", __VA_ARGS__)
// Log an error with the location
#define eflog(...) elog("[", __FILE__, ", ", __func__, ':', __LINE__, "]: ", __VA_ARGS__)
// Exit the program with the location
#define ffail(...) fail("[", __FILE__, ", ", __func__, ':', __LINE__, "]: ", __VA_ARGS__)

template<typename T, std::size_t N,
     std::enable_if<
            !std::is_same_v<char,          std::decay_t<T>> &&
            !std::is_same_v<unsigned char, std::decay_t<T>>
    >
>
std::ostream &operator<<(std::ostream &stream, const T (&array)[N]) {
    stream << '[';
    for (std::size_t i = 0; i < N - 1; ++i) {
        stream << array[i] << ", ";
    }
    stream << array[N - 1] << ']';
    return stream;
}

#endif

#endif // __CSC_LOGGER_H__

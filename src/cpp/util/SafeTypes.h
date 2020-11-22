#pragma once

#include <functional> // for std::hash
#include <type_traits>

namespace util::SafeTypes {

template <typename, typename = void>
constexpr bool is_comparable = false;
template <class T>
inline constexpr bool is_comparable<T, std::void_t<typename T::is_comparable>> = true;

template <typename, typename = void>
constexpr bool is_ordered = false;
template <class T>
inline constexpr bool is_ordered<T, std::void_t<typename T::is_ordered>> = true;

template <typename, typename = void>
constexpr bool is_incrementable = false;
template <class T>
inline constexpr bool is_incrementable<T, std::void_t<typename T::is_incrementable>> = true;

} // namespace util::SafeTypes

// ==, !=

template <typename T, typename = std::enable_if_t<util::SafeTypes::is_comparable<T>>>
constexpr auto operator==(T const& a, T const& b) -> bool {
    return a.value == b.value;
}

template <typename T, typename = std::enable_if_t<util::SafeTypes::is_comparable<T>>>
constexpr auto operator!=(T const& a, T const& b) -> bool {
    return a.value != b.value;
}

// <, >, <=, >=

template <typename T, typename = std::enable_if_t<util::SafeTypes::is_ordered<T>>>
constexpr auto operator<(T const& a, T const& b) -> bool {
    return a.value < b.value;
}
template <typename T, typename = std::enable_if_t<util::SafeTypes::is_ordered<T>>>
constexpr auto operator>(T const& a, T const& b) -> bool {
    return a.value > b.value;
}
template <typename T, typename = std::enable_if_t<util::SafeTypes::is_ordered<T>>>
constexpr auto operator<=(T const& a, T const& b) -> bool {
    return a.value <= b.value;
}
template <typename T, typename = std::enable_if_t<util::SafeTypes::is_ordered<T>>>
constexpr auto operator>=(T const& a, T const& b) -> bool {
    return a.value >= b.value;
}

// Increment/decrement operators
// see https://en.cppreference.com/w/cpp/language/operator_incdec

template <typename T, typename = std::enable_if_t<util::SafeTypes::is_incrementable<T>>>
constexpr auto operator++(T& a) -> T& {
    ++a.value;
    return a;
}

template <typename T, typename = std::enable_if_t<util::SafeTypes::is_incrementable<T>>>
constexpr auto operator++(T& a, int) -> T {
    auto cpy = a;
    ++a.value;
    return cpy;
}

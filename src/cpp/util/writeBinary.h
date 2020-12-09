#pragma once

#include <type_traits>

namespace util {

// writes any binary blob. Makes sure it
template <typename T, typename Stream>
void writeBinary(Stream& out, T t) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);

    out.write(reinterpret_cast<char const*>(&t), sizeof(T));
}

// this only works when T is trivially constructible
template <typename T, typename Stream>
[[nodiscard]] auto readBinary(Stream& in) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_constructible_v<T>);

    auto t = T();
    in.read(reinterpret_cast<char*>(&t), sizeof(T));
    return t;
}

template <typename T, typename Stream>
[[nodiscard]] auto readBinary(Stream& in, T& t) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);

    in.read(reinterpret_cast<char*>(&t), sizeof(T));
}

} // namespace util

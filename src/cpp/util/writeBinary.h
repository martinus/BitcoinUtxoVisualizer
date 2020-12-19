#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <type_traits>

namespace util {

// writes any binary blob. Makes sure it
template <size_t S, typename T>
void writeBinary(T const& t, std::string& out) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(S == sizeof(T));

    out.append(reinterpret_cast<char const*>(&t), sizeof(T));
}

template <size_t S, typename T>
void writeBinary(T const& t, std::ofstream& out) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(S == sizeof(T));

    out.write(reinterpret_cast<char const*>(&t), sizeof(T));
}

// writes any binary blob. Makes sure it
template <size_t S, size_t ArySize>
void writeArray(std::array<uint8_t, ArySize> const& ary, std::string& out) {
    // explicitely specify array size just to be sure in the calling code
    static_assert(S == ArySize);
    out.append(reinterpret_cast<char const*>(ary.data()), ArySize);
}

// writes any binary blob. Makes sure it
template <size_t S, size_t ArySize>
void writeArray(std::array<uint8_t, ArySize> const& ary, std::ofstream& out) {
    // explicitely specify array size just to be sure in the calling code
    static_assert(S == ArySize);
    out.write(reinterpret_cast<char const*>(ary.data()), ArySize);
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

template <int S, typename T, typename Stream>
void readBinary(Stream& in, T& t) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(S == sizeof(T));

    in.read(reinterpret_cast<char*>(&t), sizeof(T));
}

// reads binary into T
template <int S, typename T>
void read(char const*& in, T& t) {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(S == sizeof(T));

    std::memcpy(&t, in, sizeof(T));
    in += sizeof(T);
}

} // namespace util

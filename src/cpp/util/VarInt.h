#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string_view>

// encodes & decodes varint

namespace buv {

// writes varint into an internal buffer. Will be overwritten every call to encode()
class VarInt {
    std::array<uint8_t, 10> mData{};

public:
    template <typename T>
    [[nodiscard]] auto encode(T val) -> std::enable_if_t<std::is_unsigned_v<T>, std::string_view> {
        auto idx = size_t();
        do {
            mData[idx] = static_cast<uint8_t>(val & 0b0111'1111U);
            val >>= 7U;
            ++idx;
        } while (val);
        return std::string_view(reinterpret_cast<char const*>(mData.data()), idx);
    }

    template <typename T>
    [[nodiscard]] auto encode(T val) -> std::enable_if_t<std::is_signed_v<T>, std::string_view> {
        // zig-zag encoding of val
        using UT = std::make_unsigned_t<T>;
        auto uVal = static_cast<UT>((val << 1) ^ (val >> (sizeof(T) * 8 - 1)));
        return encode(uVal);
    }

    // TODO(martinus) decode
};

/*
template <typename Stream, typename T>
auto decodeUInt(Stream& in, T& val) -> size_t {
    static_assert(std::is_unsigned<T>::value, "only for unsigned types");

    int num_bytes = 0;
    val = 0;
    uint8_t byte;
    is.read1(byte);
    while (byte & 0b10000000) {
        val |= static_cast<T>(byte & 0b01111111) << (7 * num_bytes);
        ++num_bytes;
        is.read1(byte);
    }
    val |= static_cast<T>(byte) << (7 * num_bytes);
    return num_bytes + 1;
}
*/
} // namespace buv
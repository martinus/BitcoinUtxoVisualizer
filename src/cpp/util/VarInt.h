#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string_view>

// Encodes & decodes varint, in LEB128 format. See https://en.wikipedia.org/wiki/LEB128
//
// each byte has 7 bit of contents, and the highest bit is 1. If highest bit is 0, this is the last byte of the int.
// signed integers are zigzag-encoded.
namespace util {

// 16/7 = 2.28 => maximum 3 btes for 16bit
// 32/7 = 4.5 => maximum 5 bytes for 32bit
// 64/7 = 9.14 => maximum 10 bytes for 64bit
template <typename UT>
[[nodiscard]] auto decodeUint(char const*& ptr) -> UT {
    static_assert(std::is_unsigned_v<UT>);

    // byte 1
    auto b = static_cast<UT>(*ptr++);
    auto val = static_cast<UT>(b & 127);
    if (b < 128) {
        return val;
    };

    // byte 2
    b = *ptr++;
    val |= (b & 127) << 7;
    if (b < 128) {
        return val;
    };

    // byte 3
    b = *ptr++;
    val |= (b & 127) << (7 * 2);

    if constexpr (sizeof(UT) == 2) {
        return val;
    }
    if (b < 128) {
        return val;
    };

    // byte 4
    b = *ptr++;
    val |= (b & 127) << (7 * 3);
    if (b < 128) {
        return val;
    };

    // byte 5
    b = *ptr++;
    val |= (b & 127) << (7 * 4);

    if constexpr (sizeof(UT) > 4) {
        // can't have more than 5 bytes for uint32_t

        if (b < 128) {
            return val;
        };

        // byte 6
        b = *ptr++;
        val |= (b & 127) << (7 * 5);
        if (b < 128) {
            return val;
        };

        // byte 7
        b = *ptr++;
        val |= (b & 127) << (7 * 6);
        if (b < 128) {
            return val;
        };

        // byte 8
        b = *ptr++;
        val |= (b & 127) << (7 * 7);
        if (b < 128) {
            return val;
        };

        // byte 9
        b = *ptr++;
        val |= (b & 127) << (7 * 8);
        if (b < 128) {
            return val;
        };

        // byte 10
        b = *ptr++;
        val |= (b & 127) << (7 * 9);
    }
    return val;
}

// writes varint into an internal buffer. Will be overwritten every call to encode()
class VarInt {
    std::array<uint8_t, 10> mData{};

public:
    template <typename T>
    [[nodiscard]] auto encode(T val) -> std::enable_if_t<std::is_unsigned_v<T>, std::string_view> {
        auto idx = size_t();
        do {
            auto byte = static_cast<uint8_t>(val);
            val >>= 7U;
            mData[idx] = byte | uint8_t(0b1000'0000U);
            ++idx;
        } while (val != 0U);
        mData[idx - 1] &= uint8_t(0b0111'1111U);
        return std::string_view(reinterpret_cast<char const*>(mData.data()), idx);
    }

    template <typename T>
    [[nodiscard]] auto encode(T val) -> std::enable_if_t<std::is_signed_v<T>, std::string_view> {
        // zig-zag encoding of val
        using UT = std::make_unsigned_t<T>;
        auto uVal = static_cast<UT>((val << 1) ^ (val >> (sizeof(T) * 8 - 1)));
        return encode(uVal);
    }

    // decodes from ptr, returns the decoded value and the new iterator position.
    template <typename T>
    static void decode(T& val, char const*& ptr) {
        if constexpr (std::is_unsigned_v<T>) {
            val = decodeUint<T>(ptr);
        } else {
            // zig zag decode
            using UT = std::make_unsigned_t<T>;
            auto uVal = decodeUint<UT>(ptr);
            val = static_cast<T>((uVal >> 1U) ^ -(uVal & 1));
        }
    }
};

} // namespace util

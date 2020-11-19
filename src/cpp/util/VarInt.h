#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string_view>

// Encodes & decodes varint.
// each byte has 7 bit of contents, and the highest bit is 1. If highest bit is 0, this is the last byte of the int.
// signed integers are zigzag-encoded.
namespace util {

// writes varint into an internal buffer. Will be overwritten every call to encode()
class VarInt {
    std::array<uint8_t, 16> mData{};

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
    [[nodiscard]] static auto decode(char const* ptr) -> std::pair<T, char const*> {
        using UT = std::make_unsigned_t<T>;
        auto byte = uint8_t();
        auto uVal = UT();
        auto shift = size_t();
        do {
            byte = static_cast<uint8_t>(*ptr);
            uVal |= static_cast<UT>(byte & uint8_t(0b0111'1111U)) << shift;
            ++ptr;
            shift += 7U;
        } while (byte & uint8_t(0b1000'0000U));
        if constexpr (std::is_unsigned_v<T>) {
            return std::make_pair(uVal, ptr);
        } else {
            // zig zag decode
            auto val = static_cast<T>((uVal >> 1U) ^ -(uVal & 1));
            return std::make_pair(val, ptr);
        }
    }
};

} // namespace util

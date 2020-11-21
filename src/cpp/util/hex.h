#pragma once

#include <fmt/core.h>

#include <array>
#include <string>
#include <string_view>

namespace util {

struct Hex {
    std::string_view value{};
};

auto toHex(std::string_view input) -> std::string;

template <size_t S>
auto toHex(std::array<uint8_t, S> const& data) -> std::string {
    return toHex(std::string_view(reinterpret_cast<char const*>(data.data()), data.size()));
}

template <size_t S>
auto fromHex(char const* inputHex) -> std::array<uint8_t, S> {
    static constexpr auto digittoval = std::array<uint8_t, 256>{
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 10,  11,  12,  13,  14,  15,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

    auto data = std::array<uint8_t, S>();
    for (size_t i = 0; i < S; ++i) {
        auto hi = static_cast<uint8_t>(digittoval[inputHex[i * 2]] << 4U);
        auto lo = digittoval[inputHex[i * 2 + 1]];
        data[i] = hi | lo;
    }
    return data;
}

} // namespace util

template <>
struct fmt::formatter<util::Hex> {
    static auto parse(fmt::format_parse_context& ctx) -> format_parse_context::iterator;
    static auto format(util::Hex const& h, format_context& ctx) -> format_context::iterator;
};

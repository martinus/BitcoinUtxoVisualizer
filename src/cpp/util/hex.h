#pragma once

#include <fmt/core.h>

#include <string>
#include <string_view>

namespace util {

struct Hex {
    std::string_view value{};
};

auto hex(std::string_view input) -> std::string;

} // namespace util

template <>
struct fmt::formatter<util::Hex> {
    static auto parse(fmt::format_parse_context& ctx) -> format_parse_context::iterator;
    static auto format(util::Hex const& h, format_context& ctx) -> format_context::iterator;
};

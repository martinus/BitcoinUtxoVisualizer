#include "hex.h"

#include <fmt/format.h>

#include <string>
#include <string_view>

namespace util {

auto toHex(std::string_view input) -> std::string {
    return fmt::format("{}", Hex{input});
}

} // namespace util

namespace fmt {

auto formatter<util::Hex>::parse(format_parse_context& ctx) -> format_parse_context::iterator {
    const auto* it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
        throw format_error("invalid format");
    }
    return it;
}

auto formatter<util::Hex>::format(util::Hex const& h, format_context& ctx) -> format_context::iterator {
    auto out = ctx.out();
    for (char c : h.value) {
        static constexpr auto ary = "0123456789abcdef";
        auto byte = static_cast<uint8_t>(c);
        format_to(out, "{}{}", ary[byte >> 4U], ary[byte & 0b1111U]);
    }
    return out;
}

} // namespace fmt

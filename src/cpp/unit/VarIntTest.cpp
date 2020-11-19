#include <util/VarInt.h>

#include <doctest.h>
#include <fmt/format.h>

struct Hex {
    std::string_view value{};
};

template <>
struct fmt::formatter<Hex> {
    static constexpr auto parse(format_parse_context& ctx) {
        const auto* it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }

    template <typename FormatContext>
    auto format(Hex const& hex, FormatContext& ctx) {
        auto out = ctx.out();
        for (char c : hex.value) {
            static constexpr auto ary = "0123456789abcdef";
            auto byte = static_cast<uint8_t>(c);
            format_to(out, "{}{}", ary[byte >> 4U], ary[byte & 0b1111U]);
        }
        return out;
    }
};

auto hex(std::string_view input) -> std::string {
    return fmt::format("{}", Hex{input});
}

template <typename T>
void testEncDec(T val) {
    auto varint = util::VarInt();
    auto sv = varint.encode(val);
    auto [decVal, ptr] = util::VarInt::decode<T>(sv.data());
    REQUIRE(ptr == sv.end());
    REQUIRE(decVal == val);
}

TEST_CASE("varint_int64_t") {
    auto varint = util::VarInt();
    REQUIRE(hex(varint.encode(0)) == "00");
    REQUIRE(hex(varint.encode(-1)) == "01");
    REQUIRE(hex(varint.encode(1)) == "02");
    REQUIRE(hex(varint.encode(-2)) == "03");
    REQUIRE(hex(varint.encode(2)) == "04");

    REQUIRE(hex(varint.encode(123456)) == "80890f");
    testEncDec(127U);
    testEncDec(128U);
    testEncDec(123456U);
    testEncDec(123456);

    testEncDec(std::numeric_limits<int64_t>::min());
    testEncDec(std::numeric_limits<int64_t>::max());
    testEncDec(std::numeric_limits<int32_t>::min());
    testEncDec(std::numeric_limits<int32_t>::max());
    testEncDec(std::numeric_limits<int16_t>::min());
    testEncDec(std::numeric_limits<int16_t>::max());
    testEncDec(std::numeric_limits<int8_t>::min());
    testEncDec(std::numeric_limits<int8_t>::max());

    testEncDec(std::numeric_limits<uint64_t>::min());
    testEncDec(std::numeric_limits<uint64_t>::max());
    testEncDec(std::numeric_limits<uint32_t>::min());
    testEncDec(std::numeric_limits<uint32_t>::max());
    testEncDec(std::numeric_limits<uint16_t>::min());
    testEncDec(std::numeric_limits<uint16_t>::max());
    testEncDec(std::numeric_limits<uint8_t>::min());
    testEncDec(std::numeric_limits<uint8_t>::max());
}

#include <util/VarInt.h>
#include <util/hex.h>

#include <doctest.h>
#include <fmt/format.h>

template <typename T>
void testEncDec(T val) {
    auto varint = util::VarInt();
    auto sv = varint.encode(val);
    auto [decVal, ptr] = util::VarInt::decode<T>(sv.data());
    REQUIRE(ptr == sv.end());
    REQUIRE(decVal == val);
}

TEST_CASE("varint") {
    using util::hex;

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

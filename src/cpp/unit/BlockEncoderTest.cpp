#include <app/BlockEncoder.h>

#include <util/hex.h>

#include <doctest.h>
#include <fmt/format.h>

// creates the data structure
TEST_CASE("block_encoder_test_simple") {
    auto bs = buv::BlockEncoder();
    bs.beginBlock(123456);
    bs.endBlock();

    std::string data;
    bs.encode(data);

    REQUIRE(data.size() == 4U + 4U + 4U);
    REQUIRE(data.substr(0, 4) == "BLK\1");
}

TEST_CASE("block_encoder_test_data") {
    auto bs = buv::BlockEncoder();
    bs.beginBlock(123456);
    bs.addSpentOutput(777, 93938);
    bs.addSpentOutput(760, 93940);
    bs.addSpentOutput(789, 132);
    bs.endBlock();

    std::string data;
    bs.encode(data);
    REQUIRE(data.substr(0, 4) == "BLK\1");
}

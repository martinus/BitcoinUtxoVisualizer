#include <app/BlockEncoder.h>

#include <util/hex.h>

#include <doctest.h>
#include <fmt/format.h>

// creates the data structure
TEST_CASE("block_encoder_test_simple") {
    auto cib = buv::ChangesInBlock();
    cib.beginBlock(123456);
    cib.finalizeBlock();

    auto data = cib.encode();

    REQUIRE(data.size() == 4U + 4U + 4U);
    REQUIRE(data.substr(0, 4) == "BLK\1");
}

TEST_CASE("block_encoder_test_data") {
    auto cib = buv::ChangesInBlock();
    cib.beginBlock(123456);
    cib.addChange(93938, 777);
    cib.addChange(93940, 760);
    cib.addChange(132, 789);
    cib.finalizeBlock();

    auto data = cib.encode();
    REQUIRE(data.substr(0, 4) == "BLK\1");
}

namespace {

auto makeTwoBlocks() -> std::string {
    auto cib = buv::ChangesInBlock();
    cib.beginBlock(123456);
    cib.addChange(93938, 777);
    cib.addChange(93940, 760);
    cib.addChange(132, 789);
    cib.finalizeBlock();
    auto data = cib.encode();

    cib.beginBlock(123457);
    cib.addChange(93888, 12345);
    cib.addChange(94888, 12346);
    cib.finalizeBlock();
    data += cib.encode();

    return data;
}

} // namespace

TEST_CASE("block_encode_and_skip") {
    auto data = makeTwoBlocks();
    auto [blockHeight, ptr] = buv::ChangesInBlock::skip(data.data());
    REQUIRE(blockHeight == 123456);
    std::tie(blockHeight, ptr) = buv::ChangesInBlock::skip(ptr);
    REQUIRE(blockHeight == 123457);
    REQUIRE(ptr == data.data() + data.size());
}

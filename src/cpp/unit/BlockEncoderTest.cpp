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

auto makeTwoBlocks() -> std::vector<buv::ChangesInBlock> {
    auto cibs = std::vector<buv::ChangesInBlock>();
    auto cib = buv::ChangesInBlock();
    cib.beginBlock(123456);
    cib.addChange(93938, 777);
    cib.addChange(93940, 760);
    cib.addChange(132, 789);
    cib.finalizeBlock();
    cibs.emplace_back(cib);

    cib.beginBlock(123457);
    cib.addChange(93888, 12345);
    cib.addChange(94888, 12346);
    cib.finalizeBlock();
    cibs.emplace_back(cib);

    return cibs;
}

} // namespace

TEST_CASE("block_encode_and_skip") {
    auto cibs = makeTwoBlocks();
    auto data = cibs[0].encode() + cibs[1].encode();

    auto [blockHeight, ptr] = buv::ChangesInBlock::skip(data.data());
    REQUIRE(blockHeight == 123456);
    std::tie(blockHeight, ptr) = buv::ChangesInBlock::skip(ptr);
    REQUIRE(blockHeight == 123457);
    REQUIRE(ptr == data.data() + data.size());
}

TEST_CASE("block_encode_and_decode") {
    auto cibs = makeTwoBlocks();
    auto data = cibs[0].encode() + cibs[1].encode();

    auto [cib, ptr] = buv::ChangesInBlock::decode(data.data());
    REQUIRE(cib.blockHeight() == 123456);
    REQUIRE(cib.satoshiAndBlockheights().size() == 3);
    REQUIRE(cib.satoshiAndBlockheights()[0] == buv::SatoshiAndBlockheight(132, 789));
    REQUIRE(cib.satoshiAndBlockheights()[1] == buv::SatoshiAndBlockheight(93938, 777));
    REQUIRE(cib.satoshiAndBlockheights()[2] == buv::SatoshiAndBlockheight(93940, 760));

    std::tie(cib, ptr) = buv::ChangesInBlock::decode(ptr);
    REQUIRE(cib.blockHeight() == 123457);
    REQUIRE(cib.satoshiAndBlockheights().size() == 2);
    REQUIRE(cib.satoshiAndBlockheights()[0] == buv::SatoshiAndBlockheight(93888, 12345));
    REQUIRE(cib.satoshiAndBlockheights()[1] == buv::SatoshiAndBlockheight(94888, 12346));
}

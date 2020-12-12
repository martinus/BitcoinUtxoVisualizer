#include <app/BlockEncoder.h>

#include <util/hex.h>

#include <doctest.h>
#include <fmt/format.h>

// creates the data structure
TEST_CASE("block_encoder_test_simple") {
    auto cib = buv::ChangesInBlock();
    auto& bd = cib.beginBlock(123456);
    bd.difficulty = 1234;
    cib.finalizeBlock();

    auto data = cib.encode();

    REQUIRE(data.substr(0, 4) == "BLK\2");
}

TEST_CASE("block_encoder_test_data") {
    auto cib = buv::ChangesInBlock();
    (void)cib.beginBlock(123456);
    cib.addChange(-93938, 777);
    cib.addChange(-93940, 760);
    cib.addChange(132, 123456);
    cib.finalizeBlock();

    auto data = cib.encode();
    REQUIRE(data.substr(0, 4) == "BLK\2");
}

namespace {

auto makeBlockData(int offset, buv::BlockData& bd) {
    for (size_t i = 0; i < bd.hash.size(); ++i) {
        bd.hash[i] = static_cast<uint8_t>(offset + i);
        bd.merkleRoot[i] = static_cast<uint8_t>(offset + i + 1);
        bd.chainWork[i] = static_cast<uint8_t>(offset + i + 2);
    }
    bd.difficulty = 12345678 + offset;
    bd.time = 999999 + offset;
    bd.medianTime = 888888 + offset;
    bd.version = 123321 + offset;
    bd.nonce = 998877 + offset;
    bd.nTx = 765 + offset;
    for (size_t i = 0; i < bd.bits.size(); ++i) {
        bd.bits[i] = static_cast<uint8_t>(offset + i);
    }
    bd.size = 99988800 + offset;
    bd.strippedSize = 7775550 + offset;
    bd.weight = 919191 + offset;
}

auto makeTwoBlocks() -> std::vector<buv::ChangesInBlock> {
    auto cibs = std::vector<buv::ChangesInBlock>();
    auto cib = buv::ChangesInBlock();
    auto& bd1 = cib.beginBlock(123456);
    makeBlockData(0, bd1);

    cib.addChange(93938, 123456);
    cib.addChange(93940, 123456);
    cib.addChange(-132, 789);
    cib.finalizeBlock();
    cibs.emplace_back(cib);

    auto& bd2 = cib.beginBlock(123457);
    makeBlockData(1, bd2);

    cib.addChange(93888, 123457);
    cib.addChange(94888, 123457);
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
    auto bd = buv::BlockData();
    auto cibs = makeTwoBlocks();
    auto data = cibs[0].encode() + cibs[1].encode();

    auto [cib, ptr] = buv::ChangesInBlock::decode(data.data());
    REQUIRE(cib.blockData().blockHeight == 123456);
    bd.blockHeight = 123456;
    makeBlockData(0, bd);
    REQUIRE(cib.blockData() == bd);

    REQUIRE(cib.changeAtBlockheights().size() == 3);
    REQUIRE(cib.changeAtBlockheights()[0] == buv::ChangeAtBlockheight(-132, 789));
    REQUIRE(cib.changeAtBlockheights()[1] == buv::ChangeAtBlockheight(93938, 123456));
    REQUIRE(cib.changeAtBlockheights()[2] == buv::ChangeAtBlockheight(93940, 123456));

    std::tie(cib, ptr) = buv::ChangesInBlock::decode(ptr);
    REQUIRE(cib.blockData().blockHeight == 123457);
    bd.blockHeight = 123457;
    makeBlockData(1, bd);
    REQUIRE(cib.blockData() == bd);

    REQUIRE(cib.changeAtBlockheights().size() == 2);
    REQUIRE(cib.changeAtBlockheights()[0] == buv::ChangeAtBlockheight(93888, 123457));
    REQUIRE(cib.changeAtBlockheights()[1] == buv::ChangeAtBlockheight(94888, 123457));
}

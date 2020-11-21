#include <util/hex.h>

#include <doctest.h>

TEST_CASE("hex") {
    const auto* txIdHex = "4c84d14b6df09126db9d9634cfd85e6a0382387333c9e7a3a9bb940bb3a23fcf";

    auto txIdBin = util::fromHex<32>(txIdHex);
    REQUIRE(txIdBin.size() == 32U);

    auto txIdHex2 = util::toHex(txIdBin);
    REQUIRE(txIdHex == txIdHex2);
}

TEST_CASE("hex_small") {
    const auto* txIdHex = "4c84d14b6df09126db9d9634cfd85e6a0382387333c9e7a3a9bb940bb3a23fcf";

    auto txIdBin = util::fromHex<2>(txIdHex);
    REQUIRE(txIdBin.size() == 2U);

    auto txIdHex2 = util::toHex(txIdBin);
    REQUIRE(txIdHex2 == "4c84");
}

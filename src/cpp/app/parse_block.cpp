#include <util/BlockReader.h>

#include <doctest.h>
#include <fmt/format.h>

#include <string_view>

TEST_CASE("parse_block") {
    static constexpr auto bitcoinRpcUrl = "http://127.0.0.1:8332";
    static constexpr auto blockId = "0000000000000000000e5c22c24fd31a29d8ecb2c2cec80d3a9cb75b7aa54f18";

    auto br = util::BlockReader::create(bitcoinRpcUrl);

    for (;;) {
        auto body = br->read(blockId);
        fmt::print("{}\n", body.size());
    }
}

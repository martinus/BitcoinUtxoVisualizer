#include <util/HttpClient.h>

#include <doctest.h>
#include <fmt/format.h>

#include <string_view>

TEST_CASE("parse_block") {
    static constexpr auto bitcoinRpcUrl = "http://127.0.0.1:8332";
    static constexpr auto blockId = std::string_view("0000000000000000000e5c22c24fd31a29d8ecb2c2cec80d3a9cb75b7aa54f18");

    auto cli = util::HttpClient::create(bitcoinRpcUrl);

    auto body = cli->get("/rest/block/{}.json", blockId);
    fmt::print("{}\n", body.size());
}

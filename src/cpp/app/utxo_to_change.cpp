#include <app/BlockSerializer.h>
#include <app/Utxo.h>
#include <util/HttpClient.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/format.h>
#include <simdjson.h>

#include <filesystem>
#include <string_view>

TEST_CASE("utxo_to_change") {
    static constexpr auto bitcoinRpcUrl = "http://127.0.0.1:8332";
    static constexpr auto dataDir = "../../out/blocks";
    static constexpr auto genesisBlock = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";

    std::filesystem::create_directories(dataDir);

    fmt::print("created {}, accessing bitcoin at {}\n", dataDir, bitcoinRpcUrl);

    auto cli = util::HttpClient::create(bitcoinRpcUrl);
    auto jsonParser = simdjson::dom::parser();

    auto nextblockhash = std::string_view(genesisBlock);
    while (true) {
        auto json = cli->get("/rest/block/{}.json", nextblockhash);
        simdjson::dom::element blockData = jsonParser.parse(json);

        uint64_t height = blockData["height"];
        LOG("height={}, bytes={}", height, json.size());

        if (blockData["nextblockhash"].get(nextblockhash) != 0U) {
            // last block, stop everything
            break;
        }
    }
}

#include <app/Cfg.h>
#include <util/HttpClient.h>
#include <util/args.h>

#include <doctest.h>
#include <fmt/format.h>
#include <simdjson.h>

#include <string_view>

namespace {}

// Loads & serializes all block headers
TEST_CASE("load_all_block_headers" * doctest::skip()) {
    auto cfg = buv::parseCfg(util::args::get("-cfg").value());

    static constexpr auto bitcoinRpcUrl = "http://127.0.0.1:8332";
    static constexpr auto genesisBlock = std::string_view("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");

    auto cli = util::HttpClient::create(bitcoinRpcUrl);
    auto jsonParser = simdjson::dom::parser();

    auto block = genesisBlock;
    while (true) {
        auto json = cli->get("/rest/headers/2000/{}.json", block);
        simdjson::dom::array data = jsonParser.parse(json);

        // auto nextblockhash = std::string_view();
        for (simdjson::dom::element e : data) {
            std::string_view hash = e["hash"];
            uint64_t height = e["height"];
            uint64_t mediantime = e["mediantime"];
            double difficulty = e["difficulty"];
            int64_t nTx = e["nTx"];
            fmt::print("{}\t{}\t{}\t{}\t{}\n", hash, height, mediantime, difficulty, nTx);
        }
        simdjson::dom::element last = data.at(data.size() - 1);
        if (last["nextblockhash"].get(block) != 0U) {
            // field not found, break
            break;
        }

        // it's ok to use std::string_view for block, because it is available until the parse() call, at which point we already
        // have used it.
    }
}

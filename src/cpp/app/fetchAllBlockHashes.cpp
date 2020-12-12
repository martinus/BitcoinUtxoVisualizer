#include "fetchAllBlockHashes.h"

#include <util/HttpClient.h>
#include <util/date.h>
#include <util/hex.h>
#include <util/writeBinary.h>

#include <fmt/ostream.h>
#include <simdjson.h>

#include <chrono>
#include <fstream>

namespace buv {

auto fetchAllBlockHashes(std::unique_ptr<util::HttpClient>& cli) -> std::vector<BlockHash> {
    auto jsonParser = simdjson::dom::parser();

    // genesis block
    auto block = std::string_view("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");

    auto blockHashes = std::vector<BlockHash>();
    while (true) {
        auto json = cli->get("/rest/headers/2000/{}.json", block);
        simdjson::dom::array data = jsonParser.parse(json);

        // auto nextblockhash = std::string_view();
        for (simdjson::dom::element e : data) {
            auto hash = util::fromHex<32>(e["hash"].get_string().value().data());
            blockHashes.push_back(hash);
        }

        simdjson::dom::element last = data.at(data.size() - 1);
        if (last["nextblockhash"].get(block) != 0U) {
            // field not found, break
            break;
        }

        // it's ok to use std::string_view for block, because it is available until the parse() call, at which point we already
        // have used it.
    }

    return blockHashes;
}

auto fetchAllBlockHashes(char const* bitcoinRpcUrl) -> std::vector<BlockHash> {
    auto cli = util::HttpClient::create(bitcoinRpcUrl);
    return fetchAllBlockHashes(cli);
}

} // namespace buv

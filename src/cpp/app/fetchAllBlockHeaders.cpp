#include "fetchAllBlockHeaders.h"

#include <util/BlockHeightProgressBar.h>
#include <util/HttpClient.h>
#include <util/Throttle.h>
#include <util/date.h>
#include <util/hex.h>
#include <util/writeBinary.h>

#include <fmt/ostream.h>
#include <simdjson.h>

#include <chrono>
#include <fstream>

using namespace std::literals;

namespace buv {

auto fetchAllBlockHeaders(std::unique_ptr<util::HttpClient>& cli) -> std::vector<BlockHeader> {
    auto jsonParser = simdjson::dom::parser();
    auto throttler = util::ThrottlePeriodic(300ms);

    auto json = cli->get("/rest/chaininfo.json");
    auto numBlocks = jsonParser.parse(json)["blocks"].get_uint64().value();

    auto pb = util::BlockHeightProgressBar::create(numBlocks, "fetching block hashes ");

    // genesis block
    auto block = std::string_view("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");

    auto blockHeaders = std::vector<BlockHeader>();
    while (true) {
        auto json = cli->get("/rest/headers/2000/{}.json", block);
        simdjson::dom::array data = jsonParser.parse(json);

        // auto nextblockhash = std::string_view();
        for (simdjson::dom::element e : data) {
            auto bh = BlockHeader();
            bh.hash = util::fromHex<32>(e["hash"].get_string().value().data());
            bh.nTx = e["nTx"].get_uint64().value();
            blockHeaders.push_back(bh);
        }

        if (throttler() || blockHeaders.size() >= numBlocks) {
            pb->set_progress(blockHeaders.size(), "{}/{} blocks", blockHeaders.size(), numBlocks);
        }

        simdjson::dom::element last = data.at(data.size() - 1);
        if (last["nextblockhash"].get(block) != 0U) {
            // field not found, break
            break;
        }

        // it's ok to use std::string_view for block, because it is available until the parse() call, at which point we already
        // have used it.
    }

    return blockHeaders;
}

auto fetchAllBlockHeaders(char const* bitcoinRpcUrl) -> std::vector<BlockHeader> {
    auto cli = util::HttpClient::create(bitcoinRpcUrl);
    return fetchAllBlockHeaders(cli);
}

} // namespace buv

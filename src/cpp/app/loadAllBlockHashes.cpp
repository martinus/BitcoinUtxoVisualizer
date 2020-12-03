#include "loadAllBlockHashes.h"

#include <util/LogThrottler.h>
#include <util/log.h>

#include <simdjson.h>

using namespace std::literals;

namespace buv {

[[nodiscard]] auto loadAllBlockHashes(std::unique_ptr<util::HttpClient> const& cli) -> std::vector<std::string> {
    return loadAllBlockHashes(cli, "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
}

[[nodiscard]] auto loadAllBlockHashes(std::unique_ptr<util::HttpClient> const& cli, std::string_view startBlock)
    -> std::vector<std::string> {
    auto allBlockHashes = std::vector<std::string>();
    auto throttler = util::LogThrottler(1s);

    auto jsonParser = simdjson::dom::parser();
    auto block = startBlock;
    while (true) {
        auto json = cli->get("/rest/headers/2000/{}.json", block);
        simdjson::dom::array data = jsonParser.parse(json);

        // auto nextblockhash = std::string_view();
        for (simdjson::dom::element e : data) {
            allBlockHashes.emplace_back(e["hash"].get_string().value());
        }
        simdjson::dom::element last = data.at(data.size() - 1);
        LOGIF(throttler(), "got {} headers", allBlockHashes.size());

        if (last["nextblockhash"].get(block) != 0U) {
            // field not found, finished!
            LOG("block headers done! got {} headers", allBlockHashes.size());
            return allBlockHashes;
        }

        // it's ok to use std::string_view for block, because it is available until the parse() call, at which point we already
        // have used it.
    }
}

} // namespace buv

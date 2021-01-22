#include <app/Cfg.h>
#include <app/fetchAllBlockHeaders.h>
#include <util/args.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/format.h>
#include <simdjson.h>

#include <string_view>

namespace {}

// Loads & serializes all block headers
TEST_CASE("fetch_all_block_hashes" * doctest::skip()) {
    auto cfg = buv::parseCfg(util::args::get("-cfg").value());
    
    auto allBlockHeaders = buv::fetchAllBlockHeaders(cfg.bitcoinRpcUrl.c_str());
    LOG("got {} blocks", allBlockHeaders.size());
}

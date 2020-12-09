#include <app/BlockHeader.h>
#include <app/Cfg.h>
#include <util/args.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/format.h>
#include <simdjson.h>

#include <string_view>

namespace {}

// Loads & serializes all block headers
TEST_CASE("fetch_all_block_headers" * doctest::skip()) {
    auto cfg = buv::parseCfg(util::args::get("-cfg").value());
    auto allBlockHeaders = buv::BlockHeader::fetch(cfg.bitcoinRpcUrl.c_str());
    LOG("got {} blocks", allBlockHeaders.size());

    buv::BlockHeader::write(allBlockHeaders, cfg.blockHeadersFile);
    LOG("wrote blockheaders into '{}'", cfg.blockHeadersFile);
}

TEST_CASE("load_all_block_headers" * doctest::skip()) {
    auto cfg = buv::parseCfg(util::args::get("-cfg").value());

    LOG("Loading '{}'", cfg.blockHeadersFile);
    auto allBlockHeaders = buv::BlockHeader::load(cfg.blockHeadersFile);
    LOG("done! got {} headers.", allBlockHeaders.size());

    LOG("genesis header: {}", allBlockHeaders.first());
}

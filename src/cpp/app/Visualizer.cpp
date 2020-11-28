#include <app/forEachChange.h>
#include <buv/Density.h>
#include <util/LogThrottler.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

#include <cmath>

using namespace std::literals;

TEST_CASE("visualizer" * doctest::skip()) {
    // size_t const width = 3840;
    // size_t const height = 2160;
    // size_t const max_included_density = 444;

    auto cfg = buv::Density::Cfg();
    // cfg.pixelWidth = 2560;
    // cfg.pixelHeight = 1440;
    cfg.pixelWidth = 1600;
    cfg.pixelHeight = 1200;
    cfg.minSatoshi = 1;
    cfg.maxSatoshi = int64_t(10'000) * int64_t(100'000'000);
    cfg.minBlockHeight = 0;
    cfg.maxBlockHeight = 658'000;
    auto density = buv::Density(cfg);

    auto throttler = util::LogThrottler(1000ms);

    buv::forEachChange("../../out/blocks/changes.blk1", [&](buv::ChangesInBlock const& cib) {
        LOG_IF(throttler(), "block {}, {} changes", cib.blockHeight(), cib.changeAtBlockheights().size());

        density.begin_block(cib.blockHeight());
        //auto maxBlockAmount = int64_t(0);
        for (auto const& change : cib.changeAtBlockheights()) {
            density.change(change.blockHeight(), change.satoshi());
            //maxBlockAmount = std::max(std::abs(change.satoshi()), maxBlockAmount);
        }
        //LOG("{}: {}", cib.blockHeight(), maxBlockAmount);
        density.end_block(cib.blockHeight());
    });
}

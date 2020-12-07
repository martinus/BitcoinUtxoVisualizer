#include <app/forEachChange.h>
#include <buv/Density.h>
#include <util/Throttle.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

#include <cmath>

using namespace std::literals;

// recommended settings: https://gist.github.com/mikoim/27e4e0dc64e384adbcb91ff10a2d3678
//
// fast settings:
// ffmpeg -f rawvideo -pixel_format rgb24 -video_size 3840x2160 -framerate 60 -i "tcp://127.0.0.1:12987?listen"  -c:v libx264 -preset ultrafast out.mp4
TEST_CASE("visualizer" * doctest::skip()) {
    // size_t const width = 3840;
    // size_t const height = 2160;
    // size_t const max_included_density = 444;

    auto cfg = buv::Density::Cfg();
    cfg.pixelWidth = 3840;
    cfg.pixelHeight = 2160;
    cfg.minSatoshi = 1;
    cfg.maxSatoshi = int64_t(10'000) * int64_t(100'000'000);
    cfg.minBlockHeight = 0;
    cfg.maxBlockHeight = 658'000;
    auto density = buv::Density(cfg);

    auto throttler = util::ThrottlePeriodic(1000ms);

    buv::forEachChange("/run/media/martinus/big/bitcoin/BitcoinUtxoVisualizer/changes.blk1", [&](buv::ChangesInBlock const& cib) {
        LOGIF(throttler(), "block {}, {} changes", cib.blockHeight(), cib.changeAtBlockheights().size());

        density.begin_block(cib.blockHeight());
        // auto maxBlockAmount = int64_t(0);
        for (auto const& change : cib.changeAtBlockheights()) {
            density.change(change.blockHeight(), change.satoshi());
            // maxBlockAmount = std::max(std::abs(change.satoshi()), maxBlockAmount);
        }
        // LOG("{}: {}", cib.blockHeight(), maxBlockAmount);
        density.end_block(cib.blockHeight());
    });
}

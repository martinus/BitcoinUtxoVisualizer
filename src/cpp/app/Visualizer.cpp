#include <app/Cfg.h>
#include <app/forEachChange.h>
#include <buv/Density.h>
#include <buv/SocketStream.h>
#include <util/Throttle.h>
#include <util/args.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <simdjson.h>

#include <cmath>

using namespace std::literals;

// clang-format off
//
// 1. Start ffmpeg or ffplay (see below)
//    * ffplay -f rawvideo -pixel_format rgb24 -video_size 3840x2160 -framerate 60 -i "tcp://127.0.0.1:12987?listen"
//    * ffmpeg -f rawvideo -pixel_format rgb24 -video_size 3840x2160 -framerate 60 -i "tcp://127.0.0.1:12987?listen" -c:v libx264 -preset ultrafast out.mp4
//    * recommended settings: https://gist.github.com/mikoim/27e4e0dc64e384adbcb91ff10a2d3678
//
// 2. ninja && ./buv -ns -tc=visualizer
//
// clang-format on
TEST_CASE("visualizer" * doctest::skip()) {
    std::filesystem::path cfgFile = util::args::get("-cfg").value();

    auto cfg = buv::parseCfg(cfgFile);
    auto density = buv::Density(cfg);
    auto throttler = util::ThrottlePeriodic(1000ms);

    auto socketStream = buv::SocketStream::create(cfg.connectionIpAddr.c_str(), cfg.connectionSocket);

    buv::forEachChange(cfg.blkFile, [&](buv::ChangesInBlock const& cib) {
        LOGIF(throttler(), "block {}, {} changes", cib.blockHeight(), cib.changeAtBlockheights().size());

        density.begin_block(cib.blockHeight());
        // auto maxBlockAmount = int64_t(0);
        for (auto const& change : cib.changeAtBlockheights()) {
            density.change(change.blockHeight(), change.satoshi());
            // maxBlockAmount = std::max(std::abs(change.satoshi()), maxBlockAmount);
        }
        // LOG("{}: {}", cib.blockHeight(), maxBlockAmount);
        density.end_block(cib.blockHeight(), [&](size_t width, size_t height, uint8_t const* data) {
            socketStream->write(data, width * height * 3);
        });
    });
}

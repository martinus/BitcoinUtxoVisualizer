#include <app/Cfg.h>
#include <app/Hud.h>
#include <app/forEachChange.h>
#include <buv/Density.h>
#include <buv/SocketStream.h>
#include <util/Throttle.h>
#include <util/args.h>
#include <util/kbhit.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <simdjson.h>

#include <cmath>
#include <fstream>

using namespace std::literals;

void saveImagePPM(size_t width, size_t height, uint8_t const* data, std::string const& filename) {
    // see http://netpbm.sourceforge.net/doc/ppm.html
    std::ofstream fout(filename, std::ios::binary);
    fout << "P6\n" << width << " " << height << "\n" << 255 << "\n";
    fout.write(reinterpret_cast<char const*>(data), width * height * 3U);
}

// clang-format off
//
// 1. Start ffmpeg or ffplay (see below)
//    * ffplay -f rawvideo -pixel_format rgb24 -video_size 3840x2160 -framerate 60 -i "tcp://127.0.0.1:12987?listen"
//    * ffmpeg -f rawvideo -pixel_format rgb24 -video_size 3840x2160 -framerate 60 -i "tcp://127.0.0.1:12987?listen" -c:v libx264 -profile:v high -bf 2 -g 30 -crf 23 -pix_fmt yuv420p out.mp4
//        -crf 23: 23GB
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

    auto file = util::Mmap(cfg.blkFile);
    auto numBlocks = buv::numBlocks(file);
    LOG("{} blocks, overwritting cfg with that setting", numBlocks);
    cfg.maxBlockHeight = numBlocks - 1;

    auto hud = buv::Hud::create(cfg);
    auto socketStream = buv::SocketStream::create(cfg.connectionIpAddr.c_str(), cfg.connectionSocket);

    buv::forEachChange(file, [&](buv::ChangesInBlock const& cib) {
        LOGIF(throttler(), "block {}, {} changes", cib.blockHeight(), cib.changeAtBlockheights().size());

        density.begin_block(cib.blockHeight());
        for (auto const& change : cib.changeAtBlockheights()) {
            density.change(change.blockHeight(), change.satoshi());
        }

        auto hudInfo = buv::HudBlockInfo();
        hudInfo.blockHeight = cib.blockHeight();

        density.end_block(cib.blockHeight(), [&](uint8_t const* data) {
            hud->draw(data, hudInfo);
            socketStream->write(hud->data(), hud->size());
        });

        if (util::kbhit()) {
            switch (std::getchar()) {
            case 'q':
                // quit
                return false;

            case 's': {
                auto imgFileName = fmt::format("img_{:07}.ppm", cib.blockHeight());
                LOG("Writing image '{}'", imgFileName);
                saveImagePPM(cfg.pixelWidth, cfg.pixelHeight, hud->data(), imgFileName);
            }
            }
        }

        return true;
    });
}

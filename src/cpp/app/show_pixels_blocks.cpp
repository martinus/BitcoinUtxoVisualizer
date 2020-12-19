#include <app/Cfg.h>
#include <buv/SatoshiBlockheightToPixel.h>
#include <util/args.h>

#include <doctest.h>

[[nodiscard]] auto findNextPixel(buv::SatoshiBlockheightToPixel const& sbp, uint32_t blockHeight) -> uint32_t {
    auto x = sbp.blockheightToPixelWidth(blockHeight);
    do {
        ++blockHeight;
    } while (x == sbp.blockheightToPixelWidth(blockHeight));
    return blockHeight;
}

TEST_CASE("show_pixels_block" * doctest::skip()) {
    // show first 10 pixels
    auto cfg = buv::parseCfg(util::args::get("-cfg").value());
    auto sbp = buv::SatoshiBlockheightToPixel(cfg, 661046);

    auto blockHeight = uint32_t();
    LOG("Blockheight");
    for (auto i = size_t(); i < 10; ++i) {
        auto nextBlockHeight = findNextPixel(sbp, blockHeight);
        LOG("{:5} -{:5} : pixel {}", blockHeight, nextBlockHeight - 1, sbp.blockheightToPixelWidth(blockHeight));
        blockHeight = nextBlockHeight;
    }
}

#pragma once

#include <app/Cfg.h>
#include <buv/LinearFunction.h>
#include <buv/truncate.h>
#include <util/log.h>

#include <cmath>

namespace buv {

class SatoshiBlockheightToPixel {
    LinearFunction mFnSatoshi;
    LinearFunction mFnBlock;
    Rect<size_t> mRect{};

public:
    inline explicit SatoshiBlockheightToPixel(Cfg const& cfg, uint32_t numBlocks)
        : mFnSatoshi(std::log(static_cast<double>(cfg.maxSatoshi)),
                     0,
                     std::log(static_cast<double>(cfg.minSatoshi)),
                     static_cast<double>(cfg.graphRect.h))
        , mFnBlock(0, 0, static_cast<double>(numBlocks - 1), static_cast<double>(cfg.graphRect.w))
        , mRect(cfg.graphRect) {
        LOG("Satoshi from {}-{} -> {}-{}", cfg.maxSatoshi, cfg.minSatoshi, 0.0, static_cast<double>(cfg.graphRect.h));
        LOG("Height from {}-{} -> {}-{}", 0, numBlocks - 1, 0, static_cast<double>(cfg.graphRect.w));
    }

    [[nodiscard]] inline auto satoshiToPixelHeight(int64_t satoshi) const -> size_t {
        auto const famount = satoshi >= 0 ? satoshi : -satoshi;
        return mRect.y + truncate<size_t>(0, static_cast<size_t>(mFnSatoshi(std::log(famount))), mRect.h - 1);
    }

    [[nodiscard]] inline auto blockheightToPixelWidth(uint32_t blockHeight) const -> size_t {
        auto pixel_x = static_cast<size_t>(mFnBlock(blockHeight));
        if (pixel_x > mRect.w - 1) {
            pixel_x = mRect.w - 1;
        }
        return mRect.x + pixel_x;
    }
};

} // namespace buv

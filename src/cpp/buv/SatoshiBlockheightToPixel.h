#pragma once

#include <app/Cfg.h>
#include <buv/LinearFunction.h>
#include <buv/truncate.h>

#include <cmath>

namespace buv {

class SatoshiBlockheightToPixel {
    LinearFunction mFnSatoshi;
    LinearFunction mFnBlock;
    Rect<size_t> mRect{};

public:
    inline explicit SatoshiBlockheightToPixel(Cfg const& cfg)
        : mFnSatoshi(std::log(static_cast<double>(cfg.maxSatoshi)),
                     0.0,
                     std::log(static_cast<double>(cfg.minSatoshi)),
                     static_cast<double>(cfg.graphRect.h))
        , mFnBlock(static_cast<double>(cfg.minBlockHeight),
                   0,
                   static_cast<double>(cfg.maxBlockHeight),
                   static_cast<double>(cfg.graphRect.w))
        , mRect(cfg.graphRect) {}

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

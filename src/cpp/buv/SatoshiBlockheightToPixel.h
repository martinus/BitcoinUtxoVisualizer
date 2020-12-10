#pragma once

#include <app/Cfg.h>
#include <buv/LinearFunction.h>
#include <buv/truncate.h>

#include <cmath>

namespace buv {

class SatoshiBlockheightToPixel {
    LinearFunction mFnSatoshi;
    LinearFunction mFnBlock;
    size_t mHeight{};
    size_t mWidth{};

public:
    inline explicit SatoshiBlockheightToPixel(Cfg const& cfg)
        : mFnSatoshi(std::log(static_cast<double>(cfg.maxSatoshi)),
                     0.0,
                     std::log(static_cast<double>(cfg.minSatoshi)),
                     static_cast<double>(cfg.imageHeight))
        , mFnBlock(static_cast<double>(cfg.minBlockHeight),
                   0,
                   static_cast<double>(cfg.maxBlockHeight),
                   static_cast<double>(cfg.imageWidth))
        , mHeight(cfg.imageHeight)
        , mWidth(cfg.imageWidth) {}

    [[nodiscard]] inline auto satoshiToPixelHeight(int64_t satoshi) const -> size_t {
        auto const famount = satoshi >= 0 ? satoshi : -satoshi;
        return truncate<size_t>(0, static_cast<size_t>(mFnSatoshi(std::log(famount))), mHeight - 1);
    }

    [[nodiscard]] inline auto blockheightToPixelWidth(uint32_t blockHeight) const -> size_t {
        auto pixel_x = static_cast<size_t>(mFnBlock(blockHeight));
        if (pixel_x > mWidth - 1) {
            pixel_x = mWidth - 1;
        }
        return pixel_x;
    }
};

} // namespace buv

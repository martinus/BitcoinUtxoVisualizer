#include "Hud.h"
#include "util/hex.h"

#include <buv/SatoshiBlockheightToPixel.h>
#include <util/date.h>
#include <util/log.h>

#include <fmt/format.h>
#include <opencv2/freetype.hpp>
#include <opencv2/imgproc.hpp>

#include <cstring>

namespace {

enum class Origin {
    top_left,
    top_center,
    top_right,
    center_left,
    center,
    center_right,
    bottom_left,
    bottom_center,
    bottom_right,
};

template <typename... Args>
void write(cv::Mat& mat, size_t x, size_t y, Origin origin, char const* format, Args&&... args) {
    auto color = cv::Scalar(255, 255, 255);
    auto fontFace = cv::FONT_HERSHEY_SIMPLEX;
    auto fontScale = 0.6;
    auto thickness = 1;

    auto text = fmt::format(format, std::forward<Args>(args)...);
    auto pos = cv::Point(x, y);

    auto baseline = int();
    auto size = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
    baseline += thickness;

    // ignores baseline, so we get consistent alignment regardless of the letters used. I think. Untested.

    switch (origin) {
    case Origin::top_left:
        pos.y += size.height;
        break;
    case Origin::top_center:
        pos.x -= size.width / 2;
        pos.y += size.height;
        break;
    case Origin::top_right:
        pos.x -= size.width;
        pos.y += size.height;
        break;

    case Origin::center_left:
        pos.y += size.height / 2;
        break;
    case Origin::center:
        pos.x -= size.width / 2;
        pos.y += size.height / 2;
        break;
    case Origin::center_right:
        pos.x -= size.width;
        pos.y += size.height / 2;
        break;

    case Origin::bottom_left:
        // nothing to do, that's the default
        break;
    case Origin::bottom_center:
        pos.x -= size.width / 2;
        break;
    case Origin::bottom_right:
        pos.x -= size.width;
        break;
    }
    cv::putText(mat, text, pos, fontFace, fontScale, color, thickness, cv::LINE_AA);
}

} // namespace

namespace buv {

Hud::Hud() = default;

Hud::~Hud() = default;

class HudImpl : public Hud {
    Cfg mCfg;
    cv::Mat mMat{};
    SatoshiBlockheightToPixel mSatoshiBlockheightToPixel;

public:
    explicit HudImpl(Cfg const& cfg)
        : mCfg(cfg)
        , mMat(cfg.imageHeight, cfg.imageWidth, CV_8UC3)
        , mSatoshiBlockheightToPixel(cfg) {}

    void writeAmount(size_t x, size_t y, char const* number, char const* denom) {
        auto mid = 60;
        auto offset = 3;
        auto originNumber = Origin::center_right;
        auto originDenom = Origin::center_left;
        if (y == 0) {
            originNumber = Origin::top_right;
            originDenom = Origin::top_left;
        } else if (y >= mCfg.imageHeight - 1) {
            originNumber = Origin::bottom_right;
            originDenom = Origin::bottom_left;
        }

        write(mMat, x + mid - offset, y, originNumber, number);
        write(mMat, x + mid, y, originDenom, denom);
    }

    // Copies rgbSource, then draws dat based on the given info.
    void draw(uint8_t const* rgbSource, HudBlockInfo const& info) override {
        std::memcpy(mMat.ptr(), rgbSource, mMat.total() * mMat.elemSize());
        auto formattedTime = date::format("%F %T %Z", std::chrono::floor<std::chrono::seconds>(info.blockHeader->time));

        write(mMat, 700, 0, Origin::top_right, "{} time", formattedTime);
        write(mMat, 700, 20, Origin::top_right, "{:5} height", info.blockHeader->height);
        write(mMat, 700, 40, Origin::top_right, "{} hash", util::toHex(info.blockHeader->hash));
        write(mMat, 700, 60, Origin::top_right, "{} difficulty", info.blockHeader->difficulty);
        write(mMat, 700, 80, Origin::top_right, "{} number of transactions", info.blockHeader->nTx);

        auto oneBtc = int64_t(100'000'000);
        auto x = mSatoshiBlockheightToPixel.blockheightToPixelWidth(info.blockHeader->height);
        for (int64_t mult = 1; mult <= oneBtc * 10000; mult *= 10) {
            for (int64_t digit = 1; digit < 10; ++digit) {
                auto y = mSatoshiBlockheightToPixel.satoshiToPixelHeight(digit * mult);
                auto offset = 4;
                auto len = 5;
                if (digit == 1) {
                    len *= 2;
                }
                cv::line(mMat, cv::Point(x + offset, y), cv::Point(x + offset + len, y), cv::Scalar(255, 255, 255));
            }
        }

        // draw the legend
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(10000 * oneBtc), "10", "kBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(1000 * oneBtc), "1", "kBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(100 * oneBtc), "100", "BTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(10 * oneBtc), "10", "BTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(oneBtc), "1", "BTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(10000000), "100", "mBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(1000000), "10", "mBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(100000), "1", "mBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(10000), "100", "uBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(1000), "10", "uBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(100), "1", "uBTC");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(10), "10", "Satoshi");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(1), "1", "Satoshi");
    }

    // Returns the drawn RGB data.
    [[nodiscard]] auto data() const -> uint8_t const* override {
        return static_cast<uint8_t const*>(mMat.datastart);
    }

    [[nodiscard]] auto size() const -> size_t override {
        return mMat.total() * mMat.elemSize();
    }
};

auto Hud::create(Cfg const& cfg) -> std::unique_ptr<Hud> {
    return std::make_unique<HudImpl>(cfg);
}

} // namespace buv

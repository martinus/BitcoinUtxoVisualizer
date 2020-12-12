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

using UnixClockSeconds = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

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

template <typename... Args>
void writeMono(cv::Mat& mat, size_t x, size_t y, Origin origin, char const* format, Args&&... args) {
    auto color = cv::Scalar(255, 255, 255);
    auto fontFace = cv::FONT_HERSHEY_SIMPLEX;
    auto fontScale = 0.6;
    auto thickness = 1;

    auto text = fmt::format(format, std::forward<Args>(args)...);
    auto pos = cv::Point(x, y);

    auto baseline = int();

    // gets spacing for digit '0' and uses this for the spacing.
    auto letterSize = cv::getTextSize("0", fontFace, fontScale, thickness, &baseline);
    auto size = letterSize;
    size.width *= text.size();

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

    for (auto ch : text) {
        auto zeroTerminatedString = std::array<char, 2>();
        zeroTerminatedString[0] = ch;
        // center this letter
        auto thisLetterSize = cv::getTextSize(zeroTerminatedString.data(), fontFace, fontScale, thickness, &baseline);

        auto thisPos = pos;
        thisPos.x += (letterSize.width - thisLetterSize.width) / 2;
        cv::putText(mat, zeroTerminatedString.data(), thisPos, fontFace, fontScale, color, thickness, cv::LINE_AA);
        pos.x += letterSize.width;
    }
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

    void writeAmount(size_t x,
                     size_t y,
                     char const* number,
                     char const* denom,
                     Origin originNumber = Origin::center_right,
                     Origin originDenom = Origin::center_left) {
        auto mid = 60;
        auto offset = 3;
        write(mMat, x + mid - offset, y, originNumber, number);
        write(mMat, x + mid, y, originDenom, denom);
    }

    // prints current block info
    void writeBlockInfo(ChangesInBlock const& cib) {
        auto legendX = mSatoshiBlockheightToPixel.blockheightToPixelWidth(cib.blockData().blockHeight);
        auto const& blockHeader = cib.blockData();

        auto column1x = mCfg.imageWidth - 1100;
        if (legendX + 150 > column1x) {
            column1x = 20;
        }

        auto column2x = column1x + 960;

        auto y = 10;
        auto lineSpacing = 30;

        // see e.g.
        // https://blockstream.info/block/0000000000000000000419b60c3f5d98fc6f541896b399cb14076220a718bc25?expand
        // https://www.blockchain.com/btc/block/548847

        write(mMat, column1x, y, Origin::top_left, "Hash");
        writeMono(mMat, column2x, y, Origin::top_right, util::toHex(blockHeader.hash).c_str());
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Height");
        write(mMat, column2x, y, Origin::top_right, "{}", blockHeader.blockHeight);
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Timestamp");
        write(mMat,
              column2x,
              y,
              Origin::top_right,
              date::format("%F %T %Z", UnixClockSeconds(std::chrono::seconds(blockHeader.time))).c_str());
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Size");
        write(mMat, column2x, y, Origin::top_right, "{} B", blockHeader.size);
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Weight units");
        write(mMat, column2x, y, Origin::top_right, "{} WU", blockHeader.weight);
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Number of Transactions");
        write(mMat, column2x, y, Origin::top_right, "{}", blockHeader.nTx);
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Difficulty");
        write(mMat, column2x, y, Origin::top_right, "{}", blockHeader.difficulty());
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Merkle root");
        writeMono(mMat, column2x, y, Origin::top_right, util::toHex(blockHeader.merkleRoot).c_str());
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "chainwork");
        writeMono(mMat, column2x, y, Origin::top_right, util::toHex(blockHeader.chainWork).c_str());
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Version");
        writeMono(mMat, column2x, y, Origin::top_right, "0x{:x}", blockHeader.version);
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Bits");
        writeMono(mMat, column2x, y, Origin::top_right, "0x{}", util::toHex(blockHeader.bits));
        y += lineSpacing;

        write(mMat, column1x, y, Origin::top_left, "Nonce");
        writeMono(mMat, column2x, y, Origin::top_right, "0x{:x}", blockHeader.nonce);
        y += lineSpacing;
    }

    // Copies rgbSource, then draws dat based on the given info.
    void draw(uint8_t const* rgbSource, ChangesInBlock const& cib) override {
        std::memcpy(mMat.ptr(), rgbSource, mMat.total() * mMat.elemSize());

#if 0
        mMat = cv::Scalar(70, 0, 20);
        cv::rectangle(
            mMat, cv::Rect(mCfg.graphRect.x, mCfg.graphRect.y, mCfg.graphRect.w, mCfg.graphRect.h), cv::Scalar(0, 155, 20));
#endif
        writeBlockInfo(cib);

        auto const& blockHeader = cib.blockData();
        auto formattedTime = date::format("%F %T", UnixClockSeconds(std::chrono::seconds(blockHeader.time)));

        // draw satoshi lines
        auto x = mSatoshiBlockheightToPixel.blockheightToPixelWidth(blockHeader.blockHeight);
        // auto x = mSatoshiBlockheightToPixel.blockheightToPixelWidth(mCfg.maxBlockHeight);
        auto oneBtc = int64_t(100'000'000);
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

        // draw block lines
        auto offset = mCfg.graphRect.h + mCfg.graphRect.y + 4;
        for (uint32_t h = 0; h < mCfg.maxBlockHeight; h += 10000) {
            auto legendX = mSatoshiBlockheightToPixel.blockheightToPixelWidth(h);
            auto len = 5;
            bool showText = false;
            if (h % 100000 == 0) {
                len *= 2;
                showText = true;
            }
            cv::line(mMat, cv::Point(legendX, offset), cv::Point(legendX, offset + len), cv::Scalar(255, 255, 255));

            // only print text when distance to current line is large enough, so it's not overwritten
            auto distFromMid = std::abs(static_cast<int>(x) - static_cast<int>(legendX));
            if (showText && distFromMid > 60) {
                write(mMat, legendX, offset + len + 17, h == 0 ? Origin::top_left : Origin::top_center, "{}k", h / 1000);
            }
            if (showText && distFromMid > 190) {
                // auto formattedTime = date::format("%F", std::chrono::floor<std::chrono::seconds>(info.blockHeaders[h].time));
                auto formattedTime = std::string("TODOFIXME");
                write(
                    mMat, legendX, offset + len + 30 + 17, h == 0 ? Origin::top_left : Origin::top_center, formattedTime.c_str());
            }
        }

        // draw current block marker
        cv::line(mMat, cv::Point(x, offset), cv::Point(x, offset + 15), cv::Scalar(255, 255, 255));
        write(mMat, x, offset + 10 + 17, Origin::top_center, "{}", blockHeader.blockHeight);
        write(mMat, x, offset + 40 + 17, Origin::top_center, "{}", formattedTime);

        // draw the legend
        writeAmount(x,
                    mSatoshiBlockheightToPixel.satoshiToPixelHeight(10000 * oneBtc),
                    "10",
                    "kBTC",
                    Origin::top_right,
                    Origin::top_left);
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
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(10), "10", "sat");
        writeAmount(x, mSatoshiBlockheightToPixel.satoshiToPixelHeight(1), "1", "sat", Origin::bottom_right, Origin::bottom_left);
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

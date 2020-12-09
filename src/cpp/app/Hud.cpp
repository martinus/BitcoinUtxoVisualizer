#include "Hud.h"

#include <fmt/format.h>
#include <opencv2/freetype.hpp>
#include <opencv2/imgproc.hpp>

#include <cstring>

namespace {

enum class Origin { top_left, top_right, bottom_left, bottom_right };

template <typename... Args>
void write(cv::Mat& mat, size_t x, size_t y, Origin origin, char const* format, Args&&... args) {
    auto color = cv::Scalar(255, 255, 255);
    auto fontFace = cv::FONT_HERSHEY_SIMPLEX;
    auto fontScale = 0.7;
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
    case Origin::top_right:
        pos.x -= size.width;
        pos.y += size.height;
        break;
    case Origin::bottom_left:
        // nothing to do, that's the default
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
    cv::Mat mMat{};

public:
    explicit HudImpl(Cfg const& cfg)
        : mMat(cfg.pixelHeight, cfg.pixelWidth, CV_8UC3) {}

    // Copies rgbSource, then draws dat based on the given info.
    void draw(uint8_t const* rgbSource, HudBlockInfo const& info) override {
        std::memcpy(mMat.ptr(), rgbSource, mMat.total() * mMat.elemSize());
        write(mMat, 300, 0, Origin::top_right, "{:5} Block height", info.blockHeight);
        write(mMat, 300, 20, Origin::top_right, "{:5} whatever", info.blockHeight * 123);
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

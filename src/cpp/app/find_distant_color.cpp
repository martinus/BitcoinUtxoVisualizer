#include <buv/ColorMap.h>
#include <util/log.h>

#include <doctest.h>
#include <nanobench.h>

#include <cmath>

using namespace std::literals;

namespace {

[[nodiscard]] auto dist(uint8_t const* rgb1, uint8_t const* rgb2) -> int64_t {
    auto dr = static_cast<int64_t>(rgb1[0]) - static_cast<int64_t>(rgb2[0]);
    auto dg = static_cast<int64_t>(rgb1[1]) - static_cast<int64_t>(rgb2[1]);
    auto db = static_cast<int64_t>(rgb1[2]) - static_cast<int64_t>(rgb2[2]);
    return dr * dr + dg * dg + db * db;
}

void findBestHighlightColor(buv::ColorMapType colorType) {
    auto colormap = buv::ColorMap::create(colorType);
    auto const* colormapRgb = colormap.rgb(0);

    auto rng = ankerl::nanobench::Rng();
    auto multiplier = rng() | 1U;

    auto bestClosestDist = int64_t();
    auto bestRgb = uint32_t();
    for (uint32_t i = 0; i <= 0xFFFFFF; ++i) {
        // multiply with a random odd constant so we a more or less random rgb values. We still iterate all values.
        auto rgbVal = i * multiplier;
        auto const* rgb = reinterpret_cast<uint8_t const*>(&rgbVal);

        auto currentClosestDist = std::numeric_limits<int64_t>::max();
        for (int col = 0; col < 256; ++col) {
            auto currentDist = dist(rgb, colormapRgb + col * 3);
            if (currentDist < currentClosestDist) {
                currentClosestDist = currentDist;
                if (currentDist <= bestClosestDist) {
                    break;
                }
            }
        }

        if (currentClosestDist > bestClosestDist) {
            bestRgb = rgbVal;
            bestClosestDist = currentClosestDist;

            LOG("{:8.2f} distance for RGB({:3}, {:3}, {:3}) colormap {}",
                std::sqrt(bestClosestDist),
                rgb[0],
                rgb[1],
                rgb[2],
                to_string(colorType));
        }
    }
}

} // namespace

TEST_CASE("find_distant_color" * doctest::skip()) {
    findBestHighlightColor(buv::ColorMapType::viridis);
    findBestHighlightColor(buv::ColorMapType::magma);
    findBestHighlightColor(buv::ColorMapType::parula);
    findBestHighlightColor(buv::ColorMapType::turbo);
}

#pragma once

#include <buv/ColorMap.h>
#include <buv/truncate.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <utility>
#include <vector>

namespace buv {

// maps density data to image data. It does so continously, so we don't have to iterate
// the whole density map each time we want to extract the image.
class DensityToImage {
public:
    // initialize with background color
    // calculates the factor so that max_value is the last integer value that mapps to 255.
    DensityToImage(size_t width, size_t height, size_t max_included_value, ColorMap colormap)
        : m_colormap(std::move(colormap))
        , m_rgb(3 * width * height, 0)
        , m_fact(256.0 / std::log(max_included_value + 1))
        , m_max_included_value(max_included_value) {}

    void update(size_t pixel_idx, size_t density) {
        static constexpr auto black = std::array<uint8_t, 3>();
        uint8_t const* rgb_source = nullptr;
        if (0 == density) {
            rgb_source = black.data();
        } else if (density >= m_max_included_value) {
            rgb_source = m_colormap.rgb(255);
        } else {
            auto log = std::log(density);
            log *= m_fact;
            auto log_int = static_cast<int>(log);
            rgb_source = m_colormap.rgb(log_int);
        }
        rgb(pixel_idx, rgb_source);
    }

    void rgb(size_t pixel_idx, uint8_t const* rgb_data) {
        m_rgb[pixel_idx * 3] = rgb_data[0];
        m_rgb[pixel_idx * 3 + 1] = rgb_data[1];
        m_rgb[pixel_idx * 3 + 2] = rgb_data[2];
    }

    auto rgb(size_t pixel_idx) -> uint8_t* {
        return m_rgb.data() + (pixel_idx * 3);
    }

    [[nodiscard]] auto data() const -> uint8_t const* {
        return m_rgb.data();
    }

    // size in bytes
    [[nodiscard]] auto size() const -> size_t {
        return m_rgb.size();
    }

private:
    friend auto operator<<(std::ostream&, DensityToImage const&) -> std::ostream&;

    ColorMap const m_colormap;
    std::vector<uint8_t> m_rgb;
    double const m_fact;
    size_t const m_max_included_value;
};

inline auto operator<<(std::ostream& os, DensityToImage const& dti) -> std::ostream& {
    os.write(reinterpret_cast<const char*>(dti.m_rgb.data()), dti.m_rgb.size());
    return os;
}

} // namespace buv

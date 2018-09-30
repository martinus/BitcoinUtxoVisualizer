#pragma once

#include "ColorMap.h"
#include "truncate.h"

#include <cstdint>
#include <vector>

namespace bv {

// maps density data to image data. It does so continously, so we don't have to iterate
// the whole density map each time we want to extract the image.
class DensityToImage
{
public:
    DensityToImage(size_t width, size_t height, size_t max_included_value, ColorMap const& colormap)
        : m_colormap(colormap),
          m_width(width),
          m_height(height),
          // initialize with background color
          m_rgb(3 * width * height, 0),
          // calculates the factor so that max_value is the last integer value that mapps to 255.
          m_fact(256.0 / std::log(max_included_value + 1))
    {
    }

    void update(size_t pixel_idx, size_t density)
    {
        if (0 == density) {
            m_rgb[pixel_idx * 3] = 0;
            m_rgb[pixel_idx * 3 + 1] = 0;
            m_rgb[pixel_idx * 3 + 2] = 0;
        } else {
            auto const x = m_fact * std::log2(static_cast<double>(density));
            auto const coloridx = truncate<uint8_t>(0, x, 255);
            m_colormap.write_rgb(coloridx, &m_rgb[pixel_idx * 3]);
        }
    }

private:
    friend std::ostream& operator<<(std::ostream&, DensityToImage const&);

    ColorMap const m_colormap;
    size_t const m_width;
    size_t const m_height;
    std::vector<uint8_t> m_rgb;
    double const m_fact;
};

std::ostream& operator<<(std::ostream& os, DensityToImage const& dti)
{
    os.write(reinterpret_cast<const char*>(dti.m_rgb.data()), dti.m_rgb.size());
    return os;
}


} // namespace bv
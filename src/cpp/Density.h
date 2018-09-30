#pragma once

#include "ColorMap.h"
#include "DensityToImage.h"
#include "LinearFunction.h"
#include "truncate.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace bv {

// Integrates change data into an density image.
class Density
{
public:
    Density(size_t width, size_t height, double min_satoshi, double max_satoshi, double min_blockid, double max_blockid, size_t max_included_density_value, ColorMap const& cm)
        : m_width(width),
          m_height(height),
          m_fn_satoshi(std::log(max_satoshi), 0, std::log(min_satoshi), static_cast<double>(m_height)),
          m_fn_block(static_cast<double>(min_blockid), 0, static_cast<double>(max_blockid), static_cast<double>(m_width)),
          m_data(m_width * m_height, 0),
          m_density_image(m_width, m_height, max_included_density_value, cm)
    {
    }

    void begin_block(uint32_t block_height)
    {
    }

    void change(uint32_t block_height, int64_t amount)
    {
        size_t const pixel_x = truncate<size_t>(0, m_fn_block(block_height), m_width - 1);
        size_t const pixel_y = truncate<size_t>(0, m_fn_satoshi(std::log(amount < 0 ? -amount : amount)), m_height - 1);

        size_t const pixel_idx = pixel_y * m_width + pixel_x;
        auto& pixel = m_data[pixel_idx];
        if (amount >= 0) {
            ++pixel;
        } else {
            --pixel;
        }

        // integrate density into image
        m_density_image.update(pixel_idx, pixel);
    }

    void end_block(uint32_t block_height)
    {
    }

    // saves current status of the image as a PPM file
    void save_image_ppm(std::string filename)
    {
        // see http://netpbm.sourceforge.net/doc/ppm.html
        std::ofstream fout(filename, std::ios::binary);
        fout << "P6\n"
             << m_width << " " << m_height << "\n"
             << 255 << "\n"
             << m_density_image;
    }

private:
    size_t const m_width;
    size_t const m_height;
    LinearFunction const m_fn_satoshi;
    LinearFunction const m_fn_block;
    std::vector<size_t> m_data;

    size_t m_max_density = 0;
    DensityToImage m_density_image;
};

} // namespace bv

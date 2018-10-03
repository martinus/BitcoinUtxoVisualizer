#pragma once

#include "ColorMap.h"
#include "DensityToImage.h"
#include "LinearFunction.h"
#include "PixelSet.h"
#include "SocketStream.h"
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
    Density(size_t width, size_t height, int64_t min_satoshi, int64_t max_satoshi, double min_blockid, double max_blockid)
        : m_width(width),
          m_height(height),
          m_min_satoshi(min_satoshi),
          m_max_satoshi(max_satoshi),
          m_fn_satoshi(std::log(max_satoshi), 0, std::log(min_satoshi), static_cast<double>(m_height)),
          m_fn_block(static_cast<double>(min_blockid), 0, static_cast<double>(max_blockid), static_cast<double>(m_width)),
          m_data(m_width * m_height, 0),
          m_pixel_set(m_width * m_height),
          m_socket_stream(SocketStream::create("127.0.0.1", 12987))
    {
    }

    void begin_block(uint32_t block_height)
    {
    }

    void change(uint32_t block_height, int64_t amount)
    {
        size_t pixel_x = static_cast<size_t>(m_fn_block(block_height));
        if (pixel_x > m_width - 1) {
            pixel_x = m_width - 1;
        }

        auto const famount = amount >= 0 ? amount : -amount;
        size_t const pixel_y = truncate<size_t>(0, static_cast<size_t>(m_fn_satoshi(std::log(famount))), m_height - 1);

        size_t const pixel_idx = pixel_y * m_width + pixel_x;
        auto& pixel = m_data[pixel_idx];
        pixel += amount >= 0 ? 1 : -1;

        // integrate density into image
        //m_density_image.update(pixel_idx, pixel);
        m_pixel_set.insert(pixel_idx);
    }

    void end_block(uint32_t block_height)
    {
        //if ((block_height % 300) == 0) {
            std::cout << ".";
            std::cout.flush();

            bv::DensityToImage toi(m_width, m_height, 2000, bv::ColorMap::viridis());

            for (size_t pixel_idx = 0; pixel_idx < m_width * m_height; ++pixel_idx) {
                toi.update(pixel_idx, m_data[pixel_idx]);
            }
            for (auto pixel_idx : m_pixel_set) {
                toi.set_rgb(pixel_idx, 255, 255, 255);
            }

            m_socket_stream->write(toi.data(), toi.size());

            //std::ofstream fout("imagedata.bin", std::ios::binary | std::ios::app);
            //fout.write(toi.data(), toi.size());


            //save_image_ppm(toi, fname);
            m_pixel_set.clear();
        //}
    }

    // saves current status of the image as a PPM file
    void save_image_ppm(bv::DensityToImage& toi, std::string filename)
    {
        for (size_t pixel_idx = 0; pixel_idx < m_width * m_height; ++pixel_idx) {
            toi.update(pixel_idx, m_data[pixel_idx]);
        }
        for (auto pixel_idx : m_pixel_set) {
            toi.set_rgb(pixel_idx, 255, 255, 255);
        }

        // see http://netpbm.sourceforge.net/doc/ppm.html
        std::ofstream fout(filename, std::ios::binary);
        fout << "P6\n"
             << m_width << " " << m_height << "\n"
             << 255 << "\n"
             << toi;
    }

private:
    size_t const m_width;
    size_t const m_height;
    int64_t const m_min_satoshi;
    int64_t const m_max_satoshi;
    LinearFunction const m_fn_satoshi;
    LinearFunction const m_fn_block;
    std::vector<size_t> m_data;
    PixelSet m_pixel_set;
    std::unique_ptr<SocketStream> m_socket_stream;
};

} // namespace bv

#pragma once

#include <bv/ColorMap.h>
#include <bv/DensityToImage.h>
#include <bv/LinearFunction.h>
#include <bv/PixelSet.h>
#include <bv/PixelSetWithHistory.h>
#include <bv/SocketStream.h>
#include <bv/truncate.h>

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
          m_pixel_set_with_history(m_width * m_height, 50),
          m_current_block_pixels(m_width * m_height),
          m_socket_stream(SocketStream::create("127.0.0.1", 12987)),
          m_density_to_image(m_width, m_height, 2000, bv::ColorMap::viridis()),
          m_current_block_height(0)
    {
    }

    void begin_block(uint32_t block_height)
    {
        m_current_block_height = block_height;
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
        static size_t max_pixel_idx = 0;
        if (pixel_idx > max_pixel_idx) {
            //std::cout << pixel_x << " " << pixel_y << ", " << m_width << "x" << m_height << ", idx=" << pixel_idx << std::endl;
            max_pixel_idx = pixel_idx;
        }
        auto& pixel = m_data[pixel_idx];
        pixel += amount >= 0 ? 1 : -1;

        // integrate density into image
        //m_density_image.update(pixel_idx, pixel);
        m_current_block_pixels.insert(pixel_idx);
    }

    void end_block(uint32_t block_height)
    {
        //if (block_height < 244'000) {
        //    return;
        //}

        for (auto const pixel_idx : m_current_block_pixels) {
            m_pixel_set_with_history.insert(m_current_block_height, pixel_idx);
            m_density_to_image.update(pixel_idx, m_data[pixel_idx]);

            size_t const y = pixel_idx / m_width;
            size_t const x = pixel_idx - y * m_width;

            // upper row
            if (x > 0) {
                if (y > 0) {
                    m_pixel_set_with_history.insert(m_current_block_height - 15, (y - 1) * m_width + (x - 1));
                }
                m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 0) * m_width + (x - 1));
                if (y + 1 < m_height) {
                    m_pixel_set_with_history.insert(m_current_block_height - 15, (y + 1) * m_width + (x - 1));
                }
            }

            // middle row
            if (y > 0) {
                m_pixel_set_with_history.insert(m_current_block_height - 7, (y - 1) * m_width + (x + 0));
            }
            // m_pixel_set_with_history.insert(m_current_block_height, (y + 0) * m_width + (x + 0));
            if (y + 1 < m_height) {
                m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 1) * m_width + (x + 0));
            }

            // lower row
            if (x + 1 < m_width) {
                if (y > 0) {
                    m_pixel_set_with_history.insert(m_current_block_height - 15, (y - 1) * m_width + (x + 1));
                }
                m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 0) * m_width + (x + 1));
                if (y + 1 < m_height) {
                    m_pixel_set_with_history.insert(m_current_block_height - 15, (y + 1) * m_width + (x + 1));
                }
            }
        }
        m_pixel_set_with_history.age(block_height);


        // temporarily set all updated pixels to white
        for (auto const& blockheight_pixelidx : m_pixel_set_with_history) {
            auto rgb = m_density_to_image.rgb(blockheight_pixelidx.pixel_idx);
            int const x = block_height - blockheight_pixelidx.block_height;

            // use the inverted color as the basis
            uint8_t col[3];
            col[0] = (255 * 2 + rgb[0]) / 3;
            col[1] = (255 * 2 + rgb[1]) / 3;
            col[2] = (255 * 2 + rgb[2]) / 3;

            int const max_hist = static_cast<int>(m_pixel_set_with_history.max_history());
            rgb[0] = static_cast<uint8_t>(col[0] + ((int)rgb[0] - col[0]) * x / max_hist);
            rgb[1] = static_cast<uint8_t>(col[1] + ((int)rgb[1] - col[1]) * x / max_hist);
            rgb[2] = static_cast<uint8_t>(col[2] + ((int)rgb[2] - col[2]) * x / max_hist);

            m_density_to_image.rgb(blockheight_pixelidx.pixel_idx, rgb);
        }

        //if (block_height > 400'000) {
        m_socket_stream->write(m_density_to_image.data(), m_density_to_image.size());
        //}

        // now re-update all the updated pixels that have changed since the last update
        for (auto const& blockheight_pixelidx : m_pixel_set_with_history) {
            m_density_to_image.update(blockheight_pixelidx.pixel_idx, m_data[blockheight_pixelidx.pixel_idx]);
        }

        //save_image_ppm(toi, fname);
        //m_pixel_set.clear();
        //}

        m_current_block_pixels.clear();
    }

    // saves current status of the image as a PPM file
    void save_image_ppm(bv::DensityToImage& toi, std::string filename)
    {
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
    PixelSetWithHistory m_pixel_set_with_history;
    PixelSet m_current_block_pixels;
    std::unique_ptr<SocketStream> m_socket_stream;
    DensityToImage m_density_to_image;
    uint32_t m_current_block_height;
};

} // namespace bv

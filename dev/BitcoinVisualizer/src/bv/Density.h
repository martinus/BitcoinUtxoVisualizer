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
          m_pixel_set_with_history(m_width * m_height, 128),
          m_current_block_pixels(m_width * m_height),
          m_socket_stream(SocketStream::create("127.0.0.1", 12987)),
          m_density_to_image(m_width, m_height, 2000, bv::ColorMap::viridis()),
          m_current_block_height(0)
    {
    }

    void begin_block(uint32_t block_height)
    {
        m_current_block_height = block_height;
        m_current_block_pixels.clear();
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
        m_pixel_set_with_history.insert(m_current_block_height, pixel_idx);
        m_current_block_pixels.insert(pixel_idx);
    }

    void end_block(uint32_t block_height)
    {
        m_pixel_set_with_history.age(block_height);

        for (auto pixel_idx : m_current_block_pixels) {
            m_density_to_image.update(pixel_idx, m_data[pixel_idx]);
        }

        //if (block_height < 450000) {
        //    return;
        //}

        //if (block_height % 8 != 0) {
        //    return;
        //}

        //if ((block_height % 30) == 0) {
        // update all the updated pixels that have changed since the last update


        // temporarily set all updated pixels to white
        for (auto const& blockheight_pixelidx : m_pixel_set_with_history) {
            auto rgb = m_density_to_image.rgb(blockheight_pixelidx.pixel_idx);
            int const x = block_height - blockheight_pixelidx.block_height;

            // use the inverted color as the basis
            uint8_t col[3];
            col[0] = (255 + rgb[0]) / 2;
            col[1] = (255 + rgb[1]) / 2;
            col[2] = (255 + rgb[2]) / 2;

            int k = (int)rgb[0] - col[0];
            int diff = (k * x) / static_cast<int>(m_pixel_set_with_history.max_history());
            rgb[0] = static_cast<uint8_t>(col[0] + diff);

            k = (int)rgb[1] - col[1];
            diff = (k * x) / static_cast<int>(m_pixel_set_with_history.max_history());
            rgb[1] = static_cast<uint8_t>(col[1] + diff);

            k = (int)rgb[2] - col[2];
            diff = (k * x) / static_cast<int>(m_pixel_set_with_history.max_history());
            rgb[2] = static_cast<uint8_t>(col[2] + diff);

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

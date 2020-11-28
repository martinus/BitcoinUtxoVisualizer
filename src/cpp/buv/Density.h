#pragma once

#include <buv/ColorMap.h>
#include <buv/DensityToImage.h>
#include <buv/LinearFunction.h>
#include <buv/PixelSet.h>
#include <buv/PixelSetWithHistory.h>
#include <buv/SocketStream.h>
#include <buv/truncate.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace buv {

// Integrates change data into an density image.
class Density {
public:
    Density(size_t width, size_t height, int64_t min_satoshi, int64_t max_satoshi, double min_blockid, double max_blockid)
        : m_width(width)
        , m_height(height)
        , m_fn_satoshi(std::log(max_satoshi), 0, std::log(min_satoshi), static_cast<double>(m_height))
        , m_fn_block(static_cast<double>(min_blockid), 0, static_cast<double>(max_blockid), static_cast<double>(m_width))
        , m_data(m_width * m_height, 0)
        , m_last_data(nullptr)
        , m_pixel_set_with_history(m_width * m_height, 50)
        , m_current_block_pixels(m_width * m_height)
        , m_socket_stream(SocketStream::create("127.0.0.1", 12987))
        , m_density_to_image(m_width, m_height, 2000, ColorMap::viridis())
        , m_current_block_height(0) {}

    void begin_block(uint32_t block_height) {
        m_current_block_height = block_height;
    }

    void change(uint32_t block_height, int64_t amount, bool is_same_as_previous_change) {
        if (is_same_as_previous_change) {
            *m_last_data += amount >= 0 ? 1 : -1;
            return;
        }

        auto pixel_x = static_cast<size_t>(m_fn_block(block_height));
        if (pixel_x > m_width - 1) {
            pixel_x = m_width - 1;
        }

        auto const famount = amount >= 0 ? amount : -amount;
        auto const pixel_y = truncate<size_t>(0, static_cast<size_t>(m_fn_satoshi(std::log(famount))), m_height - 1);

        auto pixel_idx = pixel_y * m_width + pixel_x;
        static size_t max_pixel_idx = 0;
        if (pixel_idx > max_pixel_idx) {
            max_pixel_idx = pixel_idx;
        }
        m_last_data = &m_data[pixel_idx];
        *m_last_data += amount >= 0 ? 1 : -1;

        // integrate density into image
        // m_density_image.update(pixel_idx, pixel);
        m_current_block_pixels.insert(pixel_idx);
    }

    void end_block(uint32_t block_height) {
        // if (block_height < 200'000) {
        //    return;
        //}
        if (block_height >= 200'000) {
            exit(0);
        }

        for (auto const pixel_idx : m_current_block_pixels) {
            m_density_to_image.update(pixel_idx, m_data[pixel_idx]);
        }

        for (auto const pixel_idx : m_current_block_pixels) {
            size_t const y = pixel_idx / m_width;
            size_t const x = pixel_idx - y * m_width;

            // make sure we don't get an overflow!
            /*
            if (m_current_block_height >= 15 && x > 0 && x + 1 < m_width && y > 0 && y + 1 < m_height) {
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx - m_width - 1);
                m_pixel_set_with_history.insert(m_current_block_height - 07, pixel_idx - m_width);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx - m_width + 1);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx - 1);
                m_pixel_set_with_history.insert(m_current_block_height -  0, pixel_idx);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx + 1);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx + m_width - 1);
                m_pixel_set_with_history.insert(m_current_block_height - 07, pixel_idx + m_width);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx + m_width + 1);
            }
                        */

            if (m_current_block_height >= 15) {
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
                m_pixel_set_with_history.insert(m_current_block_height, pixel_idx);
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
        }
        m_pixel_set_with_history.age(block_height);

        // temporarily set all updated pixels to white
        std::vector<uint8_t> previous_rgb_values(3 * m_pixel_set_with_history.size());
        auto* previous_rgb_data = previous_rgb_values.data();

        int const max_hist = static_cast<int>(m_pixel_set_with_history.max_history());
        for (auto const& blockheight_pixelidx : m_pixel_set_with_history) {
            int const x = block_height - blockheight_pixelidx.block_height;

            int const fact = (2 * x + max_hist) / 3;
            int const opposite = (max_hist - x) / 170; // 255 * 2 / 3 = 170

            auto* rgb = m_density_to_image.rgb(blockheight_pixelidx.pixel_idx);
            *previous_rgb_data++ = rgb[0];
            *previous_rgb_data++ = rgb[1];
            *previous_rgb_data++ = rgb[2];

            rgb[0] = (rgb[0] * fact + opposite) / max_hist;
            rgb[1] = (rgb[1] * fact + opposite) / max_hist;
            rgb[2] = (rgb[2] * fact + opposite) / max_hist;
        }

        // if (block_height > 400'000) {
        m_socket_stream->write(m_density_to_image.data(), m_density_to_image.size());
        //}

        // now re-update all the updated pixels that have changed since the last update
        previous_rgb_data = previous_rgb_values.data();
        for (auto const& blockheight_pixelidx : m_pixel_set_with_history) {
            m_density_to_image.rgb(blockheight_pixelidx.pixel_idx, previous_rgb_data);
            previous_rgb_data += 3;
        }

        // save_image_ppm(toi, fname);
        // m_pixel_set.clear();
        //}

        m_current_block_pixels.clear();
    }

    // saves current status of the image as a PPM file
    void save_image_ppm(DensityToImage& toi, std::string const& filename) const {
        // see http://netpbm.sourceforge.net/doc/ppm.html
        std::ofstream fout(filename, std::ios::binary);
        fout << "P6\n" << m_width << " " << m_height << "\n" << 255 << "\n" << toi;
    }

private:
    size_t const m_width;
    size_t const m_height;
    LinearFunction const m_fn_satoshi;
    LinearFunction const m_fn_block;
    std::vector<size_t> m_data;
    size_t* m_last_data;
    PixelSetWithHistory m_pixel_set_with_history;
    PixelSet m_current_block_pixels;
    std::unique_ptr<SocketStream> m_socket_stream;
    DensityToImage m_density_to_image;
    uint32_t m_current_block_height;
};

} // namespace buv

#pragma once

#include <buv/ColorMap.h>
#include <buv/DensityToImage.h>
#include <buv/LinearFunction.h>
#include <buv/PixelSet.h>
#include <buv/PixelSetWithHistory.h>
#include <buv/SocketStream.h>
#include <buv/truncate.h>
#include <util/log.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace buv {

// Integrates change data into an density image.
class Density {
public:
    struct Cfg {
        size_t pixelWidth{};
        size_t pixelHeight{};
        int64_t minSatoshi{};
        int64_t maxSatoshi{};
        uint32_t minBlockHeight{};
        uint32_t maxBlockHeight{};
    };

    explicit Density(Cfg const& cfg)
        : mCfg(cfg)
        , m_fn_satoshi(std::log(static_cast<double>(cfg.maxSatoshi)),
                       0.0,
                       std::log(static_cast<double>(cfg.minSatoshi)),
                       static_cast<double>(cfg.pixelHeight))
        , m_fn_block(static_cast<double>(cfg.minBlockHeight),
                     0,
                     static_cast<double>(cfg.maxBlockHeight),
                     static_cast<double>(cfg.pixelWidth))
        , m_data(cfg.pixelWidth * cfg.pixelHeight, 0)
        , m_last_data(nullptr)
        , m_pixel_set_with_history(cfg.pixelWidth * cfg.pixelHeight, 50)
        , m_current_block_pixels(cfg.pixelWidth * cfg.pixelHeight)
        , m_socket_stream(SocketStream::create("127.0.0.1", 12987))
        , m_density_to_image(cfg.pixelWidth, cfg.pixelHeight, 2000, ColorMap::viridis())
        , m_current_block_height(0)
        , m_prev_block_height(-1)
        , m_prev_amount(-1) {}

    void begin_block(uint32_t block_height) {
        m_current_block_height = block_height;
    }

    void change(uint32_t block_height, int64_t amount) {
        if (amount == 0) {
            return;
        }
        if (amount == m_prev_amount && block_height == m_prev_block_height) {
            *m_last_data += amount >= 0 ? 1 : -1;
            return;
        }

        auto pixel_x = static_cast<size_t>(m_fn_block(block_height));
        if (pixel_x > mCfg.pixelWidth - 1) {
            pixel_x = mCfg.pixelWidth - 1;
        }

        auto const famount = amount >= 0 ? amount : -amount;
        auto const pixel_y = truncate<size_t>(0, static_cast<size_t>(m_fn_satoshi(std::log(famount))), mCfg.pixelHeight - 1);

        auto pixel_idx = pixel_y * mCfg.pixelWidth + pixel_x;
        static size_t max_pixel_idx = 0;
        if (pixel_idx > max_pixel_idx) {
            max_pixel_idx = pixel_idx;
        }
        m_last_data = &m_data[pixel_idx];
        *m_last_data += amount >= 0 ? 1 : -1;

        // integrate density into image
        // m_density_image.update(pixel_idx, pixel);
        m_current_block_pixels.insert(pixel_idx);

        m_prev_amount = amount;
        m_prev_block_height = block_height;
    }

    void end_block(uint32_t block_height) {
        if (block_height < 657'000) {
            return;
        }

        for (auto const pixel_idx : m_current_block_pixels) {
            m_density_to_image.update(pixel_idx, m_data[pixel_idx]);
        }

        for (auto const pixel_idx : m_current_block_pixels) {
            size_t const y = pixel_idx / mCfg.pixelWidth;
            size_t const x = pixel_idx - y * mCfg.pixelWidth;

            // make sure we don't get an overflow!
            /*
            if (m_current_block_height >= 15 && x > 0 && x + 1 < cfg.pixelWidth && y > 0 && y + 1 < cfg.pixelHeight) {
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx - cfg.pixelWidth - 1);
                m_pixel_set_with_history.insert(m_current_block_height - 07, pixel_idx - cfg.pixelWidth);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx - cfg.pixelWidth + 1);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx - 1);
                m_pixel_set_with_history.insert(m_current_block_height -  0, pixel_idx);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx + 1);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx + cfg.pixelWidth - 1);
                m_pixel_set_with_history.insert(m_current_block_height - 07, pixel_idx + cfg.pixelWidth);
                m_pixel_set_with_history.insert(m_current_block_height - 15, pixel_idx + cfg.pixelWidth + 1);
            }
                        */

            if (m_current_block_height >= 15) {
                // upper row
                if (x > 0) {
                    if (y > 0) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y - 1) * mCfg.pixelWidth + (x - 1));
                    }
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 0) * mCfg.pixelWidth + (x - 1));
                    if (y + 1 < mCfg.pixelHeight) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y + 1) * mCfg.pixelWidth + (x - 1));
                    }
                }

                // middle row
                if (y > 0) {
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y - 1) * mCfg.pixelWidth + (x + 0));
                }
                m_pixel_set_with_history.insert(m_current_block_height, pixel_idx);
                if (y + 1 < mCfg.pixelHeight) {
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 1) * mCfg.pixelWidth + (x + 0));
                }

                // lower row
                if (x + 1 < mCfg.pixelWidth) {
                    if (y > 0) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y - 1) * mCfg.pixelWidth + (x + 1));
                    }
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 0) * mCfg.pixelWidth + (x + 1));
                    if (y + 1 < mCfg.pixelHeight) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y + 1) * mCfg.pixelWidth + (x + 1));
                    }
                }
            }
        }
        m_pixel_set_with_history.age(block_height);

        // temporarily set all updated pixels to white
        std::vector<uint8_t> previous_rgb_values(3 * m_pixel_set_with_history.size());
        auto* rgb_data = previous_rgb_values.data();

        for (auto const& blockheight_pixelidx : m_pixel_set_with_history) {
            auto* rgb = m_density_to_image.rgb(blockheight_pixelidx.pixel_idx);

            rgb_data[0] = rgb[0];
            rgb_data[1] = rgb[1];
            rgb_data[2] = rgb[2];
            rgb_data += 3;
            int const x = block_height - blockheight_pixelidx.block_height;

            // use the inverted color as the basis
            int const max_hist = static_cast<int>(m_pixel_set_with_history.max_history());
            int const fact = (2 * x + max_hist) / 3;
            int const opposite = 255 * 2 * (max_hist - x) / 3;

            rgb[0] = (rgb[0] * fact + opposite) / max_hist;
            rgb[1] = (rgb[1] * fact + opposite) / max_hist;
            rgb[2] = (rgb[2] * fact + opposite) / max_hist;
        }

        // if (block_height > 400'000) {
        m_socket_stream->write(m_density_to_image.data(), m_density_to_image.size());
        //}

        // now re-update all the updated pixels that have changed since the last update
        rgb_data = previous_rgb_values.data();
        for (auto const& blockheight_pixelidx : m_pixel_set_with_history) {
            m_density_to_image.rgb(blockheight_pixelidx.pixel_idx, rgb_data);
            rgb_data += 3;
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
        fout << "P6\n" << mCfg.pixelWidth << " " << mCfg.pixelHeight << "\n" << 255 << "\n" << toi;
    }

private:
    Cfg const mCfg;
    LinearFunction const m_fn_satoshi;
    LinearFunction const m_fn_block;
    std::vector<size_t> m_data;
    size_t* m_last_data;
    PixelSetWithHistory m_pixel_set_with_history;
    PixelSet m_current_block_pixels;
    std::unique_ptr<SocketStream> m_socket_stream;
    DensityToImage m_density_to_image;
    uint32_t m_current_block_height;

    uint32_t m_prev_block_height;
    int64_t m_prev_amount;
};

} // namespace buv

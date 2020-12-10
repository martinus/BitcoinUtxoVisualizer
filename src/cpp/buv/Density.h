#pragma once

#include <app/Cfg.h>
#include <buv/ColorMap.h>
#include <buv/DensityToImage.h>
#include <buv/PixelSet.h>
#include <buv/PixelSetWithHistory.h>
#include <buv/SatoshiBlockheightToPixel.h>
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
    explicit Density(Cfg const& cfg)
        : mCfg(cfg)
        , mSatoshiBlockheightToPixel(cfg)
        , m_data(cfg.imageWidth * cfg.imageHeight, 0)
        , m_last_data(nullptr)
        , m_pixel_set_with_history(cfg.imageWidth * cfg.imageHeight, 50)
        , m_current_block_pixels(cfg.imageWidth * cfg.imageHeight)
        , m_density_to_image(
              cfg.imageWidth, cfg.imageHeight, cfg.colorUpperValueLimit, ColorMap::create(cfg.colorMap), cfg.colorBackgroundRGB)
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

        auto const pixel_x = mSatoshiBlockheightToPixel.blockheightToPixelWidth(block_height);
        auto const pixel_y = mSatoshiBlockheightToPixel.satoshiToPixelHeight(amount);

        auto pixel_idx = pixel_y * mCfg.imageWidth + pixel_x;
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

    template <typename Op>
    void end_block(uint32_t block_height, Op op) {
        if (block_height < mCfg.startShowAtBlockHeight) {
            return;
        }

        if (mCfg.skipBlocks > 1 && (block_height % mCfg.skipBlocks != 0)) {
            return;
        }

        for (auto const pixel_idx : m_current_block_pixels) {
            m_density_to_image.update(pixel_idx, m_data[pixel_idx]);
        }

        for (auto const pixel_idx : m_current_block_pixels) {
            size_t const y = pixel_idx / mCfg.imageWidth;
            size_t const x = pixel_idx - y * mCfg.imageWidth;

            if (m_current_block_height >= 15) {
                // upper row
                if (x > 0) {
                    if (y > 0) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y - 1) * mCfg.imageWidth + (x - 1));
                    }
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 0) * mCfg.imageWidth + (x - 1));
                    if (y + 1 < mCfg.imageHeight) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y + 1) * mCfg.imageWidth + (x - 1));
                    }
                }

                // middle row
                if (y > 0) {
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y - 1) * mCfg.imageWidth + (x + 0));
                }
                m_pixel_set_with_history.insert(m_current_block_height, pixel_idx);
                if (y + 1 < mCfg.imageHeight) {
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 1) * mCfg.imageWidth + (x + 0));
                }

                // lower row
                if (x + 1 < mCfg.imageWidth) {
                    if (y > 0) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y - 1) * mCfg.imageWidth + (x + 1));
                    }
                    m_pixel_set_with_history.insert(m_current_block_height - 7, (y + 0) * mCfg.imageWidth + (x + 1));
                    if (y + 1 < mCfg.imageHeight) {
                        m_pixel_set_with_history.insert(m_current_block_height - 15, (y + 1) * mCfg.imageWidth + (x + 1));
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
            int const age = block_height - blockheight_pixelidx.block_height;

            int const max_hist = static_cast<int>(m_pixel_set_with_history.max_history());

            // linear interpolate between colorHighlightRGB (0 age), and original color (max_hist age)
            rgb[0] = (rgb[0] * age + mCfg.colorHighlightRGB[0] * (max_hist - age)) / max_hist;
            rgb[1] = (rgb[1] * age + mCfg.colorHighlightRGB[1] * (max_hist - age)) / max_hist;
            rgb[2] = (rgb[2] * age + mCfg.colorHighlightRGB[2] * (max_hist - age)) / max_hist;
        }
        op(m_density_to_image.data());

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
    void save_image_ppm(std::string const& filename) const {
        // see http://netpbm.sourceforge.net/doc/ppm.html
        std::ofstream fout(filename, std::ios::binary);
        fout << "P6\n" << mCfg.imageWidth << " " << mCfg.imageHeight << "\n" << 255 << "\n" << m_density_to_image;
    }

    ~Density() {
        // sort & print pixel densities
        m_data.erase(std::remove(m_data.begin(), m_data.end(), 0), m_data.end());
        std::sort(m_data.begin(), m_data.end());

        // print 100 values
        LOG("100 density values, starting from 0 (min) to max", m_data.size());
        auto numValues = size_t(100);
        for (size_t i = 0; i < numValues; ++i) {
            fmt::print("{}, ", m_data[(m_data.size() - 1) * i / (numValues - 1)]);
        }
    }

private:
    Cfg const mCfg;
    SatoshiBlockheightToPixel mSatoshiBlockheightToPixel;
    std::vector<size_t> m_data;
    size_t* m_last_data;
    PixelSetWithHistory m_pixel_set_with_history;
    PixelSet m_current_block_pixels;
    DensityToImage m_density_to_image;
    uint32_t m_current_block_height;

    uint32_t m_prev_block_height;
    int64_t m_prev_amount;
};

} // namespace buv

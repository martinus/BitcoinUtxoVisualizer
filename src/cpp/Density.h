#pragma once

#include "LinearFunction.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace bv {

// Integrates change data into an density image.
class Density
{
public:
    Density(size_t width, size_t height, double min_satoshi, double max_satoshi, double min_blockid, double max_blockid)
        : m_width(width),
          m_height(height),
          m_fn_satoshi(std::log(max_satoshi), 0, std::log(min_satoshi), static_cast<double>(m_height)),
          m_fn_block(static_cast<double>(min_blockid), 0, static_cast<double>(max_blockid), static_cast<double>(m_width)),
          m_data(m_width * m_height, 0)
    {
    }

    void begin_block(uint32_t block_height)
    {
    }

    void change(uint32_t block_height, int64_t amount)
    {
        auto famount = amount;
        if (amount < 0) {
            famount = -amount;
        }

        size_t pixel_x = truncate<size_t>(0, m_fn_block(block_height), m_width - 1);
        size_t pixel_y = truncate<size_t>(0, m_fn_satoshi(std::log(famount)), m_height - 1);

        auto& pixel = m_data[pixel_y * m_width + pixel_x];
        if (amount >= 0) {
            ++pixel;
        } else {
            --pixel;
        }
    }

    void end_block(uint32_t block_height)
    {
    }

private:
    size_t const m_width;
    size_t const m_height;
    LinearFunction const m_fn_satoshi;
    LinearFunction const m_fn_block;
    std::vector<size_t> m_data;

    size_t m_max_density = 0;

    template <typename T>
    static T truncate(T min, double val, T max)
    {
        if (val <= min) {
            return min;
        }
        if (val >= max) {
            return max;
        }
        return static_cast<T>(val);
    }
};

} // namespace bv

#pragma once

#include <cstdint>
#include <vector>

namespace bv {

// Basically a set for pixels. Fast if the number of changed pixels is small.
//
// Quick O(1) to set a pixel
// Quick O(n) to iterate all set n pixel.
// Quick O(n) to clear all set pixels.
class PixelSet
{
public:
    PixelSet(size_t size)
        : m_pixel(size, 0)
    {
    }

    // Assumes that idx < size. O(1) operation.
    void insert(size_t pixel_idx)
    {
        if (m_pixel[pixel_idx]) {
            return;
        }
        m_pixel[pixel_idx] = 1;
        m_pixelidx.push_back(pixel_idx);
    }

    std::vector<size_t>::const_iterator begin() const
    {
        return m_pixelidx.begin();
    }
    std::vector<size_t>::const_iterator end() const
    {
        return m_pixelidx.end();
    }

    void clear()
    {
        for (auto pixel_idx : m_pixelidx) {
            m_pixel[pixel_idx] = 0;
        }
        m_pixelidx.clear();
    }

private:
    std::vector<uint8_t> m_pixel;
    std::vector<size_t> m_pixelidx;
};

} // namespace bv
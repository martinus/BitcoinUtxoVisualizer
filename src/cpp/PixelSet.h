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
        : m_pixel_is_set(size, 0)
    {
    }

    // Assumes that idx < size. O(1) operation.
    void insert(size_t idx)
    {
        if (m_pixel_is_set[idx]) {
            // already set, do nothing
            return;
        }

        m_pixel_is_set[idx] = 1;
        m_pixel_indices.push_back(idx);
    }

    void clear()
    {
        for (auto idx : m_pixel_indices) {
            m_pixel_is_set[idx] = 0;
        }
        m_pixel_indices.clear();
    }

    std::vector<size_t>::const_iterator begin() const
    {
        return m_pixel_indices.begin();
    }

    std::vector<size_t>::const_iterator end() const
    {
        return m_pixel_indices.end();
    }

private:
    std::vector<uint8_t> m_pixel_is_set;
    std::vector<size_t> m_pixel_indices;
};

} // namespace bv
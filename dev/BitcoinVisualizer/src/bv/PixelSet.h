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
    struct BlockheightPixelidx {
        uint32_t block_height;
        size_t pixel_idx;

        BlockheightPixelidx(uint32_t block_height, size_t pixel_idx)
            : block_height(block_height), pixel_idx(pixel_idx)
        {
        }
    };
    using BlockheightPixelCollection = std::vector<BlockheightPixelidx>;

    PixelSet(size_t size, size_t max_history)
        : m_max_history(max_history), m_pixel(size, sentinel)
    {
    }

    // Assumes that idx < size. O(1) operation.
    void insert(uint32_t block_height, size_t pixel_idx)
    {
        if (sentinel == m_pixel[pixel_idx]) {
            // not set: create entry
            m_pixel[pixel_idx] = m_blockheight_pixelidx.size();
            m_blockheight_pixelidx.emplace_back(block_height, pixel_idx);
        } else {
            // pixel already set: update it
            m_blockheight_pixelidx[m_pixel[pixel_idx]].block_height = block_height;
        }
    }

    // remove all pixels older than max age
    void age(uint32_t const current_block_height)
    {
        size_t idx = m_blockheight_pixelidx.size();
        while (idx--) {
            auto& pos_at_idx = m_blockheight_pixelidx[idx];
            if (pos_at_idx.block_height + m_max_history < current_block_height) {
                // clear that pixel
                m_pixel[pos_at_idx.pixel_idx] = sentinel;

                // move last entry to the now vacant position (if we are not at the end)
                if (idx != m_blockheight_pixelidx.size() - 1) {
                    pos_at_idx = std::move(m_blockheight_pixelidx.back());
                    m_pixel[pos_at_idx.pixel_idx] = idx;
                }

                // get rid of moved entry
                m_blockheight_pixelidx.pop_back();
            }
        }
    }

    BlockheightPixelCollection::const_iterator begin() const
    {
        return m_blockheight_pixelidx.begin();
    }
    BlockheightPixelCollection::const_iterator end() const
    {
        return m_blockheight_pixelidx.end();
    }

    void clear()
    {
        std::fill(m_pixel.begin(), m_pixel.end(), sentinel);
        m_blockheight_pixelidx.clear();
    }

    size_t max_history() const
    {
        return m_max_history;
    }

private:
    static const size_t sentinel = std::numeric_limits<size_t>::max();
    size_t const m_max_history;

    std::vector<size_t> m_pixel;
    BlockheightPixelCollection m_blockheight_pixelidx;
};

} // namespace bv
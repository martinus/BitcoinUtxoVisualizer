#pragma once

#include <cstdint>
#include <vector>

namespace buv {

// Basically a set for pixels. Fast if the number of changed pixels is small.
//
// Quick O(1) to set a pixel
// Quick O(n) to iterate all set n pixel.
// Quick O(n) to clear all set pixels.
class PixelSet {
    std::vector<uint8_t> mPixel{};
    std::vector<size_t> mPixelidx{};

public:
    explicit PixelSet(size_t size)
        : mPixel(size, 0) {}

    // Assumes that idx < size. O(1) operation.
    void insert(size_t pixel_idx) {
        if (mPixel[pixel_idx] != 0U) {
            return;
        }
        mPixel[pixel_idx] = 1;
        mPixelidx.push_back(pixel_idx);
    }

    [[nodiscard]] auto begin() const -> std::vector<size_t>::const_iterator {
        return mPixelidx.begin();
    }

    [[nodiscard]] auto end() const -> std::vector<size_t>::const_iterator {
        return mPixelidx.end();
    }

    void clear() {
        std::fill(mPixel.begin(), mPixel.end(), 0);
        mPixelidx.clear();
    }
};

} // namespace buv
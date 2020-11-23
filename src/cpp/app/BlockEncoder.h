#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace buv {

// Satoshi can be negative and positive. Negative means it the transaction was spent, positive means it was added.
// Obviously, changes from past blocks are always negative (always spent), and from current block it will be mostly positive, but
// some can also be spent within the same block.
class ChangeAtBlockheight {
    int64_t mSatoshi{};
    uint32_t mBlockHeight{};

public:
    constexpr ChangeAtBlockheight(int64_t satoshi, uint32_t blockHeight)
        : mSatoshi(satoshi)
        , mBlockHeight(blockHeight) {}

    [[nodiscard]] constexpr auto satoshi() const noexcept -> int64_t {
        return mSatoshi;
    }

    [[nodiscard]] constexpr auto blockHeight() const noexcept -> uint32_t {
        return mBlockHeight;
    }

    [[nodiscard]] constexpr auto operator<(ChangeAtBlockheight const& other) const noexcept -> bool {
        if (mSatoshi != other.mSatoshi) {
            return mSatoshi < other.mSatoshi;
        }
        return mBlockHeight < other.mBlockHeight;
    }

    [[nodiscard]] constexpr auto operator==(ChangeAtBlockheight const& other) const noexcept -> bool {
        return mSatoshi == other.mSatoshi && mBlockHeight == other.mBlockHeight;
    }

    [[nodiscard]] constexpr auto operator!=(ChangeAtBlockheight const& other) const noexcept -> bool {
        return !(*this == other);
    }
};

// Encodes & decodes block change data
//
// The format is designed to be compact, fast to parse, and contain all (amounts, blockheight) tuples of the current block.
// a .blk file contains concatenated entries of that payload:
//
// clang-format off
//
// field size | description  | data type | commment
// -----------|--------------|-----------|---
//          4 | marker       | string    | magic marker "BLK\1". uint32_t with value 0x014b4c42. Always exactly the same.
//          4 | block_height | uint32_t  | current block height, successively increasing value
//          4 | num_bytes    | uint32_t  | size in bytes of the data blob for this blob. Add num_bytes to skip to the next block, then you will point to the marker "BLK\0" of the next block. This is useful to quickly skip to the next block without parsing the data.
//         1+ | amount       | var_int   | var-int (zig-zag encoding) of the smallest change (most likely a negative number)
//         1+ | block        | var_uint  | var-uint encoded of the block height of the amount
//  The following fields are repeated until the end of the block (num_bytes - (8 + 4) bytes). Sorted by amount.
//         1+ | amount_diff  | var_uint  | var-uint encoded difference to previous amount. Guaranteed to be positive due to the sorting.
//         1+ | block_diff   | var_int   | var-int (zig-zag encoding) of block difference to previous entry.
//
// clang-format on
class ChangesInBlock {
    uint32_t mBlockHeight{};
    std::vector<ChangeAtBlockheight> mChangeAtBlockheights{};
    bool mIsFinalized = false;

public:
    void beginBlock(uint32_t blockHeight);
    void addChange(int64_t satoshi, uint32_t blockHeight);
    void finalizeBlock();
    [[nodiscard]] auto encode() const -> std::string;

    [[nodiscard]] auto operator==(ChangesInBlock const& other) const noexcept -> bool;
    [[nodiscard]] auto operator!=(ChangesInBlock const& other) const noexcept -> bool;

    [[nodiscard]] auto blockHeight() const noexcept -> uint32_t;
    [[nodiscard]] auto changeAtBlockheights() const noexcept -> std::vector<ChangeAtBlockheight> const&;

    // decodes the whole changesInBlock, and returns also pointer to the next block.
    [[nodiscard]] static auto decode(char const* ptr) -> std::pair<ChangesInBlock, char const*>;

    // skips the whole block, and returns the skipped block's blockHeight, and pointer to the next block
    [[nodiscard]] static auto skip(char const* ptr) -> std::pair<uint32_t, char const*>;
};

} // namespace buv

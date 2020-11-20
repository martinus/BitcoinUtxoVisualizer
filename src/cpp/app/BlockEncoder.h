#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace buv {

class SatoshiAndBlockheight {
    uint64_t mSatoshi{};
    uint32_t mBlockHeight{};

public:
    SatoshiAndBlockheight(uint64_t satoshi, uint32_t blockHeight);
    [[nodiscard]] auto satoshi() const noexcept -> uint64_t;
    [[nodiscard]] auto blockHeight() const noexcept -> uint32_t;

    [[nodiscard]] auto operator<(SatoshiAndBlockheight const& other) const noexcept -> bool;
    [[nodiscard]] auto operator==(SatoshiAndBlockheight const& other) const noexcept -> bool;
    [[nodiscard]] auto operator!=(SatoshiAndBlockheight const& other) const noexcept -> bool;
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
//          4 | block height | uint32_t  | current block height, successively increasing value
//          4 | num_bytes    | uint32_t  | size in bytes of the data blob for this blob. Add num_bytes to skip to the next block, then you will point to the marker "BLK\0" of the next block. This is useful to quickly skip to the next block without parsing the data.
//  The following fields are repeated until the end of the block (num_bytes - (8 + 4) bytes). Sorted by amount.
//         1+ | amount_diff  | var_uint  | var-uint encoded difference to previous amount. Guaranteed to be positive due to the sorting.
//         1+ | block_diff   | var_int   | var-int (zig-zag encoding) of block difference to previous entry.
//
// clang-format on
class ChangesInBlock {
    uint32_t mBlockHeight{};
    std::vector<SatoshiAndBlockheight> mSatoshiAndBlockheights{};
    bool mIsFinalized = false;

public:
    void beginBlock(uint32_t blockHeight);
    void addChange(uint64_t satoshi, uint32_t blockHeight);
    void finalizeBlock();
    [[nodiscard]] auto encode() const -> std::string;

    [[nodiscard]] auto operator==(ChangesInBlock const& other) const noexcept -> bool;
    [[nodiscard]] auto operator!=(ChangesInBlock const& other) const noexcept -> bool;

    [[nodiscard]] auto blockHeight() const noexcept -> uint32_t;
    [[nodiscard]] auto satoshiAndBlockheights() const noexcept -> std::vector<SatoshiAndBlockheight> const&;

    // decodes the whole changesInBlock, and returns also pointer to the next block.
    [[nodiscard]] static auto decode(char const* ptr) -> std::pair<ChangesInBlock, char const*>;

    // skips the whole block, and returns the skipped block's blockHeight, and pointer to the next block
    [[nodiscard]] static auto skip(char const* ptr) -> std::pair<uint32_t, char const*>;
};

} // namespace buv

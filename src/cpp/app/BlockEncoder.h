#pragma once

#include <fmt/format.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
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

struct BlockData {
    // fields are ordered by alignment requirements (not size)
    // 4
    uint32_t time{};
    uint32_t medianTime{};
    uint32_t blockHeight{};
    uint32_t version{};
    uint32_t nonce{};
    uint32_t nTx{};
    uint32_t size{};
    uint32_t strippedSize{};
    uint32_t weight{};

    // 1

    // difficulty as array so it has unique object representation. floating point sucks.
    std::array<uint8_t, sizeof(double)> difficultyArray{};
    std::array<uint8_t, 4> bits{};
    std::array<uint8_t, 32> hash{};
    std::array<uint8_t, 32> merkleRoot{};
    std::array<uint8_t, 32> chainWork{};

    [[nodiscard]] auto difficulty() const -> double {
        auto d = double();
        std::memcpy(&d, difficultyArray.data(), sizeof(double));
        return d;
    }

    void difficulty(double d) {
        std::memcpy(difficultyArray.data(), &d, sizeof(double));
    }
};

[[nodiscard]] inline auto operator==(BlockData const& a, BlockData const& b) -> bool {
    static_assert(std::has_unique_object_representations_v<BlockData>);
    return 0 == std::memcmp(&a, &b, sizeof(BlockData));
}

[[nodiscard]] inline auto operator!=(BlockData const& a, BlockData const& b) -> bool {
    return !(a == b);
}

// Encodes & decodes block change data
//
// The format is designed to be compact, fast to parse, and contain all (amounts, blockheight) tuples of the current block.
// a .blk file contains concatenated entries of that payload:
//
// clang-format off
//
// field size | description  | data type | commment
// -----------|--------------|-----------|---
//          4 | marker       | string    | magic marker "BLK\x02". Always exactly the same.
//          4 | block_height | uint32_t  | current block height, successively increasing value
//          4 | num_bytes    | uint32_t  | size in bytes of the data blob for this blob. Add num_bytes to skip to the next block, then you will point to the marker "BLK\0" of the next block. This is useful to quickly skip to the next block without parsing the data.
// header info
//         32 | hash         | binary    | 
//         32 | merkleroot   | binary    | 
//         32 | chainwork    | binary    | 
//          8 | difficulty   | var_uint  | e.g. "19314656404097"
//          4 | version      | uint32_t  | e.g. "0x20000000"
//          4 | time         | uint32_t  | unix timestamp in seconds
//          4 | mediantime   | uint32_t  | unix timestamp in seconds
//          4 | nonce        | uint32_t  |
//          4 | bits         | binary    | e.g. "0x170e92aa"
//         1+ | nTx          | var_uint  | number of transactions
//         1+ | size         | var_uint  | e.g. 1595162, "1595.162 KB" (/1000)
//         1+ | strippedsize | var_uint  | e.g. 799405
//         1+ | weight       | var_uint  | E.g. "3993377 WU" (or 3993.377 KWU, or 998345 vB (3993377 + 3)/4). See https://en.bitcoin.it/wiki/Weight_units
// transaction info
//         1+ | amount       | var_int   | var-int (zig-zag encoding) of the smallest change (most likely a negative number)
//         1+ | block        | var_uint  | var-uint encoded of the block height of the amount
//  The following fields are repeated until for all spent amounts (satoshi <= 0) Sorted by amount.
//         1+ | amount_diff  | var_uint  | var-uint encoded difference to previous amount. Guaranteed to be positive due to the sorting.
//         1+ | block_diff   | var_int   | var-int (zig-zag encoding) of block difference to previous entry.
//  The following fields are repeated until for all new amounts (satoshi > 0). Sorted by amount. No need for blockheight because it must be the current block.
//         1+ | amount_diff  | var_uint  | var-uint encoded difference to previous amount. Guaranteed to be positive due to the sorting.
//
// clang-format on
class ChangesInBlock {
    std::vector<ChangeAtBlockheight> mChangeAtBlockheights{};
    BlockData mBlockData{};
    bool mIsFinalized = false;

    size_t mNumUtxoDestroyed{};
    size_t mNumUtxoCreated{};

public:
    [[nodiscard]] auto beginBlock(uint32_t blockHeight) -> BlockData&;

    void addChange(int64_t satoshi, uint32_t blockHeight);
    void finalizeBlock();

    // sorting is automatically don in finalizeBlock. It might be beneficial though to call sort() even when later more change is
    // added, so that the final sort() is faster.
    void sort();
    
    [[nodiscard]] auto encode() const -> std::string;

    [[nodiscard]] auto operator==(ChangesInBlock const& other) const noexcept -> bool;
    [[nodiscard]] auto operator!=(ChangesInBlock const& other) const noexcept -> bool;

    [[nodiscard]] auto blockData() const noexcept -> BlockData const&;
    [[nodiscard]] auto changeAtBlockheights() const noexcept -> std::vector<ChangeAtBlockheight> const&;
    [[nodiscard]] auto numUtxoDestroyed() const noexcept -> size_t;
    [[nodiscard]] auto numUtxoCreated() const noexcept -> size_t;

    // decodes the whole changesInBlock, and returns also pointer to the next block.
    [[nodiscard]] static auto decode(char const* ptr) -> std::pair<ChangesInBlock, char const*>;
    [[nodiscard]] static auto decode(ChangesInBlock&& reusableChanges, char const* ptr) -> std::pair<ChangesInBlock, char const*>;

    // skips the whole block, and returns the skipped block's blockHeight, and pointer to the next block
    [[nodiscard]] static auto skip(char const* ptr) -> std::pair<uint32_t, char const*>;
};

} // namespace buv

template <>
struct fmt::formatter<buv::BlockData> {
    static auto parse(fmt::format_parse_context& ctx) -> format_parse_context::iterator;
    static auto format(buv::BlockData const& bd, format_context& ctx) -> format_context::iterator;
};

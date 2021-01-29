#pragma once

#include <array>
#include <cstdint>
#include <cstring> // memcpy
#include <limits>
#include <list>
#include <vector>

namespace buv {

// Utxo requires a hell of a lot of memory. So here is an idea how to bring down memory usage, at the cost of runtime performance
// (not a problem, since the limiting factor is bitcoind anyways)
//
// Map from 8 bytes txid prefix to a container with nr+satoshi entries.
// * Key is limited to 8 bytes, which should be enough to prevent any collision.
// * value is 4 bytes block height + 8 bytes ptr to chunk.
//
// A chunk is a linked list of multiple (voutnr, satoshi) tuples. (2, 8) bytes, and a ptr to the next chunk:
// [ chunk_ptr, vout0, satoshi0, vout1, satoshi1, ..., vout11, satoshi11]
// 8 + (2+8)*12 = 128 bytes (8 bytes ptr overhead)
// a vout with value 65535 is the sentinel (no more entries.). No vout==65535 but chunk_ptr==nullptr means all entries are taken,
// but no next one.
//
// A ChunkStore allocates multiple chunks at once, and gives free ones out, and takes them back. It never frees anything, just
// keeps them in a linked list.

// Stores vout and satoshi in 8 bytes.
// Special vout value 0xFFFF means it's empty.
class VoutSatoshi {
    static constexpr auto emptyMask = uint64_t(0x00000000'0000FFFF);

    // 6 byte satoshi, 2 bytes vout
    uint64_t mVoutAndSatoshi = emptyMask;

public:
    constexpr VoutSatoshi() noexcept = default;

    inline VoutSatoshi(uint16_t vout, int64_t satoshi) noexcept
        : mVoutAndSatoshi(static_cast<uint64_t>(satoshi) << 16U | vout) {}

    [[nodiscard]] constexpr auto satoshi() const noexcept -> int64_t {
        return mVoutAndSatoshi >> 16U;
    }

    [[nodiscard]] constexpr auto vout() const noexcept -> uint16_t {
        return static_cast<uint16_t>(mVoutAndSatoshi);
    }

    [[nodiscard]] constexpr auto operator==(VoutSatoshi const& other) const -> bool {
        return mVoutAndSatoshi == other.mVoutAndSatoshi;
    }

    [[nodiscard]] constexpr auto isVout(uint16_t vout) const -> bool {
        return static_cast<uint16_t>(mVoutAndSatoshi) == vout;
    }

    // internal data
    [[nodiscard]] auto data() const -> uint64_t {
        return mVoutAndSatoshi;
    }

    [[nodiscard]] auto isEmptyMask() const -> bool {
        return mVoutAndSatoshi == emptyMask;
    }
};

// Chunk contains multiple VoutSatoshi, and links to the next one.
// How many entries per chunk? The overwhelming number of chunks contain a single entry:
//
//     count    vouts
//     44820    11
//     50353    10
//     55485    9
//     66058    8
//     74970    7
//    114230    6
//    131804    5
//    229230    4
//    445223    3
//   1987081    2
//   2015402    12
//  19460719    1
//
// According to my overhead calculation, 2 elements are space-wise the optimium. Below that q
class Chunk {
    Chunk* mNextChunk = nullptr;
    VoutSatoshi mVoutSatoshi{};

public:
    [[nodiscard]] constexpr auto voutSatoshi() -> VoutSatoshi& {
        return mVoutSatoshi;
    }
    [[nodiscard]] constexpr auto voutSatoshi() const -> VoutSatoshi const& {
        return mVoutSatoshi;
    }

    constexpr void clear() {
        mVoutSatoshi = {};
        mNextChunk = nullptr;
    }

    // finds an entry with fout, returns index or numeric_limits<size_t>::max() if not found. Does NOT follow next().
    [[nodiscard]] constexpr auto isVout(uint16_t vout) const -> bool {
        return mVoutSatoshi.isVout(vout);
    }

    [[nodiscard]] constexpr auto next() -> Chunk* {
        return mNextChunk;
    }

    [[nodiscard]] constexpr auto next() const -> Chunk const* {
        return mNextChunk;
    }

    constexpr void next(Chunk* c) {
        mNextChunk = c;
    }
};

// Holds lots of bulk data in a list, and chunks in a freelist. On VoutSatoshi const& vsinsert, potentially new Chunks are
// allocated and put into the freelist. On remove chunks are put into the freelist.
class ChunkStore {
    static constexpr auto NumChunksPerBulk = 100 * 1024 * 1024 / sizeof(Chunk);
    std::list<std::array<Chunk, NumChunksPerBulk>> mStore{};
    Chunk* mFreeList = nullptr;
    size_t mNumFreeChunks = 0;

public:
    // Inserts vs into the chunk. If c is full, it gets a new chunk from the store links to it from c.
    // @return The chunk where it has inserted.
    auto insert(uint16_t vout, int64_t satoshi, Chunk* c) -> Chunk*;

    // Removes the entry in the chunklist with the given vout. Might put a free chunk back into the store. Replaces the removed
    // entry with the last enry.
    // @return The removed satoshi value, and either c or nullptr if that was the last element
    auto remove(uint16_t vout, Chunk* c) -> std::pair<int64_t, Chunk*>;

    template <typename Op>
    auto removeAllSorted(std::vector<uint16_t> const& vouts, Chunk* root, Op&& op) {
        Chunk* preFoundChunk = nullptr;
        auto* newRoot = root;
        auto* foundChunk = root;

        auto voutIt = vouts.begin();
        auto voutEnd = vouts.end();
        while (voutIt != voutEnd) {
            // preconditions:
            // * voutIt is valid
            // * foundChunk is valid, and the next element that might match
            // * preFoundChunk is either nullptr (if foundChunk is the first entry), or the previous
            if (foundChunk->isVout(*voutIt)) {
                op(foundChunk->voutSatoshi().satoshi());
                ++voutIt;

                // put foundChunk back into the store and forward
                auto* tmp = foundChunk->next();
                foundChunk->voutSatoshi() = {};
                putIntoStore(foundChunk);
                foundChunk = tmp;

                // reconnect list
                if (preFoundChunk != nullptr) {
                    preFoundChunk->next(foundChunk);
                } else {
                    newRoot = foundChunk;
                }
            } else {
                preFoundChunk = foundChunk;
                foundChunk = foundChunk->next();
            }
        }

        return newRoot;
    }

    // Number of entries in the freelist. O(1) operation
    [[nodiscard]] auto numFreeChunks() const -> size_t;

    // Number of total chunks allocated. O(1) operation
    [[nodiscard]] auto numAllocatedChunks() const -> size_t;

    // Number of chunks in one bulk allocation
    [[nodiscard]] auto numAllocatedBulks() const -> size_t;

    // Number of chunks in one bulk allocation
    [[nodiscard]] static auto numChunksPerBulk() -> size_t;

private:
    // Gets a chunk from the store. This never fails, it will allocate if none is yet available.
    auto takeFromStore() -> Chunk*;

    // Puts a chunk back into store. Does not check links.
    void putIntoStore(Chunk* chunk);
};

} // namespace buv

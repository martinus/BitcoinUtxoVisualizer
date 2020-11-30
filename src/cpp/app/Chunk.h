#pragma once

#include <array>
#include <cstdint>
#include <cstring> // memcpy
#include <limits>
#include <list>

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

// Stores vout and satoshi in 10 bytes.
// Special vout value 0xFFFF means it's empty.
class VoutSatoshi {
    static constexpr auto NumBytesSatoshi = sizeof(int64_t);

    uint16_t mVout = 0xFFFF;
    std::array<uint8_t, NumBytesSatoshi> mSatoshi{};

public:
    constexpr VoutSatoshi() noexcept = default;

    inline VoutSatoshi(uint16_t vout, int64_t x) noexcept
        : mVout(vout) {
        std::memcpy(mSatoshi.data(), &x, NumBytesSatoshi);
    }

    [[nodiscard]] inline auto satoshi() const noexcept -> int64_t {
        auto x = int64_t();
        std::memcpy(&x, mSatoshi.data(), NumBytesSatoshi);
        return x;
    }

    [[nodiscard]] inline auto vout() const noexcept -> uint16_t {
        return mVout;
    }

    [[nodiscard]] inline auto empty() const noexcept -> bool {
        return mVout == 0xFFFF;
    }
};

// Chunk contains multiple VoutSatoshi, and links to the next one.
class Chunk {
    Chunk* mNextChunk = nullptr;
    std::array<VoutSatoshi, 12> mVoutSatoshi{};

public:
    [[nodiscard]] auto voutSatoshi() noexcept -> std::array<VoutSatoshi, 12>& {
        return mVoutSatoshi;
    }

    void clear() {
        mVoutSatoshi = {};
        mNextChunk = nullptr;
    }

    [[nodiscard]] auto full() const noexcept -> bool {
        return mVoutSatoshi.back().empty();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return mVoutSatoshi.front().empty();
    }

    // finds an entry with fout, returns index or numeric_limits<size_t>::max() if not found. Does NOT follow next().
    [[nodiscard]] auto find(uint16_t vout) const -> size_t {
        for (auto i = size_t(); i < mVoutSatoshi.size(); ++i) {
            if (mVoutSatoshi[i].empty()) {
                return std::numeric_limits<size_t>::max();
            }
            if (mVoutSatoshi[i].vout() == vout) {
                return i;
            }
        }
        return std::numeric_limits<size_t>::max();
    }

    // number of elements that are taken
    [[nodiscard]] auto size() const -> size_t {
        auto s = mVoutSatoshi.size();
        while (s != 0 && mVoutSatoshi[s - 1].empty()) {
            --s;
        }
        return s;
    }

    [[nodiscard]] auto next() const -> Chunk* {
        return mNextChunk;
    }

    void next(Chunk* c) {
        mNextChunk = c;
    }
};

// Holds lots of bulk data in a list, and chunks in a freelist. On VoutSatoshi const& vsinsert, potentially new Chunks are
// allocated and put into the freelist. On remove chunks are put into the freelist.
class ChunkStore {
    std::list<std::array<Chunk, 1024 * 1024 / sizeof(Chunk)>> mStore{};
    Chunk* mFreeList = nullptr;

public:
    // Inserts vs into the chunk. If c is full, it gets a new chunk from the store links to it from c.
    // @return The chunk where it has inserted.
    auto insert(uint16_t vout, int64_t satoshi, Chunk* c) -> Chunk*;

    // Removes the entry in the chunklist with the given vout. Might put a free chunk back into the store. Replaces the removed
    // entry with the last enry.
    // @return The removed satoshi value, and either c or nullptr if that was the last element
    auto remove(uint16_t vout, Chunk* c) -> std::pair<int64_t, Chunk*>;

private:
    // Gets a chunk from the store. This never fails, it will allocate if none is yet available.
    auto takeFromStore() -> Chunk*;

    // Puts a chunk back into store. Does not check links.
    void putIntoStore(Chunk* chunk);
};

} // namespace buv

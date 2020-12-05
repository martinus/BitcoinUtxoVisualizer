#include "Chunk.h"

#include <util/log.h>

#include <fmt/format.h>

#include <stdexcept>

namespace buv {

auto ChunkStore::numFreeChunks() const -> size_t {
    return mNumFreeChunks;
}

auto ChunkStore::numAllocatedChunks() const -> size_t {
    return mStore.size() * NumChunksPerBulk;
}

auto ChunkStore::numChunksPerBulk() -> size_t {
    return NumChunksPerBulk;
}

auto ChunkStore::numAllocatedBulks() const -> size_t {
    return mStore.size();
}

auto ChunkStore::takeFromStore() -> Chunk* {
    // If freelist is empty, allocate a new array, interlink everything, and put into freelist
    if (mFreeList == nullptr) {
        auto& bulk = mStore.emplace_back();
        for (size_t i = 0; i < bulk.size() - 1; ++i) {
            bulk[i].next(&bulk[i + 1]);
        }
        mFreeList = &bulk[0];
        mNumFreeChunks += bulk.size();
    }

    // get a chunk from the freelist, and unlink it
    --mNumFreeChunks;
    auto* freeChunk = mFreeList;
    mFreeList = freeChunk->next();
    freeChunk->next(nullptr);
    // LOG("take {}", static_cast<void*>(freeChunk));
    return freeChunk;
}

void ChunkStore::putIntoStore(Chunk* c) {
    // LOG("put {}", static_cast<void*>(c));
    ++mNumFreeChunks;
    c->clear();
    c->next(mFreeList);
    mFreeList = c;
}

// Currently assumes that either:
// * c is nullptr
// * c is full, but next is nullptr
// * c is not full
auto ChunkStore::insert(uint16_t vout, int64_t satoshi, Chunk* c) -> Chunk* {
    if (c == nullptr) {
        // nothing there yet, take a new chunk
        c = takeFromStore();
    } else {
        // search forward to the last entry of the list
        while (c->next() != nullptr) {
            c = c->next();
        }

        // link a new chunk
        c->next(takeFromStore());
        c = c->next();
    }

    // now c has place for one entry, insert it at the back
    c->voutSatoshi() = {vout, satoshi};
    return c;
}

// Removes the entry in the chunklist with the given vout. Returns the removed satoshi, and the new root or nullptr if empty.
auto ChunkStore::remove(uint16_t vout, Chunk* const c) -> std::pair<int64_t, Chunk*> {
    Chunk* preFoundChunk = nullptr;
    auto* foundChunk = c;
    while (foundChunk != nullptr && !foundChunk->isVout(vout)) {
        preFoundChunk = foundChunk;
        foundChunk = foundChunk->next();
    }
    if (foundChunk == nullptr) {
        throw std::runtime_error(fmt::format("could not find vout {} in chunk {}", vout, static_cast<void*>(c)));
    }

    auto retVal = foundChunk->voutSatoshi().satoshi();
    foundChunk->voutSatoshi() = {};

    if (preFoundChunk != nullptr) {
        // unlink foundChunk
        preFoundChunk->next(foundChunk->next());
        putIntoStore(foundChunk);
        return std::make_pair(retVal, c);
    }

    // preFoundChunk is nullptr, that means foundChunk == c
    auto* newRoot = foundChunk->next();
    putIntoStore(foundChunk);
    return std::make_pair(retVal, newRoot);
}

} // namespace buv

#include "Chunk.h"
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

auto ChunkStore::takeFromStore() -> Chunk* {
    // If freelist is empty, allocate a new array, interlink everything, and put into freelist
    if (mFreeList == nullptr) {
        auto bulk = mStore.emplace_back();
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
    return freeChunk;
}

void ChunkStore::putIntoStore(Chunk* c) {
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
        while (c->full() && c->next() != nullptr) {
            c = c->next();
        }
        if (c->full()) {
            // last entry full? link a new chunk
            c->next(takeFromStore());
            c = c->next();
        }
    }

    // now c has place for one entry, insert it at the back
    for (auto& voutSatoshi : c->voutSatoshi()) {
        if (voutSatoshi.empty()) {
            voutSatoshi = {vout, satoshi};
            return c;
        }
    }

    // can't ever happen
    throw std::runtime_error("could not find an empty spot!");
}

// Removes the entry in the chunklist with the given vout. Might put a free chunk back into the store. Replaces the removed
// entry with the last enry.
// @return The removed satoshi value, and nullptreither the chunk or nullptr if the chunk was empty.
auto ChunkStore::remove(uint16_t vout, Chunk* const c) -> std::pair<int64_t, Chunk*> {
    Chunk* preFoundChunk = nullptr;
    auto* foundChunk = c;
    auto foundIdx = size_t();
    while (foundChunk != nullptr) {
        foundIdx = foundChunk->find(vout);
        if (foundIdx != std::numeric_limits<size_t>::max()) {
            break;
        }
        preFoundChunk = foundChunk;
        foundChunk = foundChunk->next();
    }
    if (foundChunk == nullptr) {
        throw std::runtime_error("could not find vout!");
    }

    // Now we have preFoundChunk, foundChunk, foundIdx. Do the same for lastChunk

    auto* preLastChunk = preFoundChunk;
    auto* lastChunk = foundChunk;
    while (lastChunk->next() != nullptr) {
        preLastChunk = lastChunk;
        lastChunk = lastChunk->next();
    }
    auto lastIdx = lastChunk->size() - 1;

    // Now we have preLastChunk, lastChunk, lastIdx. Everything we need to replace and remove!

    auto retVal = std::exchange(foundChunk->voutSatoshi(foundIdx), lastChunk->voutSatoshi(lastIdx));
    lastChunk->voutSatoshi(lastIdx) = {};

    // if lastChunk is empty, unlink it
    if (lastChunk->empty()) {
        if (preLastChunk != nullptr) {
            preLastChunk->next(nullptr);
        }
        putIntoStore(lastChunk);
        if (lastChunk == c) {
            // whole list is empty!
            return std::pair<int64_t, Chunk*>(retVal.satoshi(), nullptr);
        }
    }

    return std::make_pair(retVal.satoshi(), c);
}

} // namespace buv

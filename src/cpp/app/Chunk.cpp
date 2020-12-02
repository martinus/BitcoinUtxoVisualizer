#include "Chunk.h"
#include <stdexcept>

namespace buv {

auto ChunkStore::takeFromStore() -> Chunk* {
    // If freelist is empty, allocate a new array, interlink everything, and put into freelist
    if (mFreeList == nullptr) {
        auto bulk = mStore.emplace_back();
        for (size_t i = 0; i < bulk.size() - 1; ++i) {
            bulk[i].next(&bulk[i + 1]);
        }
        mFreeList = &bulk[0];
    }

    // get a chunk from the freelist, and unlink it
    auto* freeChunk = mFreeList;
    mFreeList = freeChunk->next();
    freeChunk->next(nullptr);
    return freeChunk;
}

void ChunkStore::putIntoStore(Chunk* c) {
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
auto ChunkStore::remove(uint16_t vout, Chunk* c) -> std::pair<int64_t, Chunk*> {
    auto* foundChunk = (Chunk*){};
    auto foundIdx = size_t();
    while (c != nullptr) {
        foundIdx = c->find(vout);
        if (foundIdx != std::numeric_limits<size_t>::max()) {
            // found the entry!
            foundChunk = c;
            break;
        }
        c = c->next();
    }

    if (foundChunk == nullptr) {
        throw std::runtime_error("could not find vout!");
    }

    // forward to last entry that contains something
    auto* lastChunk = foundChunk;
    auto* prevChunk = (Chunk*){};
    while (lastChunk->next() != nullptr) {
        prevChunk = lastChunk;
        lastChunk = lastChunk->next();
    }

    auto lastIdx = lastChunk->size() - 1;
    auto retVal = std::exchange(foundChunk->voutSatoshi(foundIdx), lastChunk->voutSatoshi(lastIdx));
    lastChunk->voutSatoshi(lastIdx) = {};
    if (lastChunk->empty()) {
        // lastChunk is now empty, put it back into the store.
        if (prevChunk != nullptr) {
            prevChunk->next(nullptr);
        }
        putIntoStore(lastChunk);
        if (lastChunk == c) {
            // whole list is empty!
            return std::make_pair(retVal.satoshi(), nullptr);
        }
    }

    return std::make_pair(retVal.satoshi(), c);
}

} // namespace buv

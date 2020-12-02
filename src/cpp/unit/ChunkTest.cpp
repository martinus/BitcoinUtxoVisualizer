#include <app/Chunk.h>

#include <doctest.h>
#include <nanobench.h>

#include <tuple>
#include <unordered_map>

TEST_CASE("chunk_single") {
    auto chunkStore = buv::ChunkStore();

    auto* chunk = chunkStore.insert(12, 12345, nullptr);
    REQUIRE(chunk->next() == nullptr);
    auto* chunk2 = chunkStore.insert(13, 4444, chunk);
    REQUIRE(chunk == chunk2);

    REQUIRE(chunk->size() == 2);
    REQUIRE(chunk->voutSatoshi(0) == buv::VoutSatoshi{12, 12345});
    REQUIRE(chunk->voutSatoshi(1) == buv::VoutSatoshi{13, 4444});

    // remove first entry
    auto [satoshi, newChunk] = chunkStore.remove(12, chunk);
    REQUIRE(satoshi == 12345);
    REQUIRE(newChunk == chunk);
    REQUIRE(chunk->size() == 1);
    REQUIRE(!chunk->empty());

    // last entry moved forward
    REQUIRE(chunk->voutSatoshi(0) == buv::VoutSatoshi{13, 4444});
    REQUIRE(chunk->voutSatoshi(1) == buv::VoutSatoshi{});
    std::tie(satoshi, newChunk) = chunkStore.remove(13, chunk);
    REQUIRE(newChunk == nullptr);
    REQUIRE(satoshi == 4444);
}

TEST_CASE("chunk_random") {
    auto voutAndSatoshi = std::vector<buv::VoutSatoshi>();
    for (uint16_t i = 0; i < 14; ++i) {
        voutAndSatoshi.emplace_back(i, i * 1'000'000);
    }

    // now fill up chunks
    auto chunkStore = buv::ChunkStore();
    buv::Chunk* baseChunk = nullptr;
    buv::Chunk* lastChunk = nullptr;
    for (auto const& vs : voutAndSatoshi) {
        lastChunk = chunkStore.insert(vs.vout(), vs.satoshi(), lastChunk);
        if (baseChunk == nullptr) {
            baseChunk = lastChunk;
        }
    }

    // now that we have plenty of data, remove until empty and check that we get exactly the same result as the vector.
    buv::Chunk* newBaseChunk = baseChunk;
    auto rng = ankerl::nanobench::Rng(123);
    while (!voutAndSatoshi.empty()) {
        auto idx = rng.bounded(voutAndSatoshi.size());
        auto removed = voutAndSatoshi[idx];
        voutAndSatoshi[idx] = voutAndSatoshi.back();
        voutAndSatoshi.pop_back();

        auto satoshi = int64_t();
        std::tie(satoshi, newBaseChunk) = chunkStore.remove(removed.vout(), newBaseChunk);
        if (voutAndSatoshi.empty()) {
            REQUIRE(newBaseChunk == nullptr);
        } else {
            REQUIRE(newBaseChunk == baseChunk);
        }
        REQUIRE(satoshi == removed.satoshi());
    }
}
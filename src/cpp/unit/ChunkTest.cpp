#include <app/Chunk.h>

#include <doctest.h>

#include <tuple>

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

#include <app/Chunk.h>

#include <doctest.h>

TEST_CASE("chunk") {
    auto chunkStore = buv::ChunkStore();

    auto* chunk = chunkStore.insert(12, 12345, nullptr);
    auto* chunk2 = chunkStore.insert(13, 4444, chunk);
    REQUIRE(chunk == chunk2);

    REQUIRE(chunk->size() == 2);
    REQUIRE(chunk->voutSatoshi(0) == buv::VoutSatoshi{12, 12345});
    REQUIRE(chunk->voutSatoshi(1) == buv::VoutSatoshi{13, 4444});
}


#include <app/Chunk.h>

#include <doctest.h>

TEST_CASE("chunk") {
    auto chunkStore = buv::ChunkStore();
    auto* chunk = chunkStore.insert(12, 12345, nullptr);
    REQUIRE(chunk != nullptr);
}

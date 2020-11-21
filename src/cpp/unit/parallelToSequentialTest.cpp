#include <util/log.h>
#include <util/parallelToSequential.h>

#include <doctest.h>

#include <chrono>
#include <thread>

using namespace std::literals;

TEST_CASE("parallel_to_sequential") {
    static constexpr auto numItems = 10000;
    auto hasIdProcessed = std::vector<uint8_t>(numItems, 0);

    auto expectedSequenceNumber = size_t();
    util::parallelToSequential(
        std::thread::hardware_concurrency(),
        numItems,
        [&](size_t /*workerId*/, size_t sequenceId) {
            REQUIRE(hasIdProcessed[sequenceId] == 0);
            hasIdProcessed[sequenceId] = 1;
        },
        [&](size_t /*workerId*/, size_t sequenceId) {
            REQUIRE(sequenceId == expectedSequenceNumber);
            REQUIRE(hasIdProcessed[sequenceId] == 1);
            REQUIRE(++hasIdProcessed[sequenceId]);
            ++expectedSequenceNumber;
        });

    for (auto b : hasIdProcessed) {
        REQUIRE(b == 2);
    }
}

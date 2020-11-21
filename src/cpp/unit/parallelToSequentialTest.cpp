#include <util/log.h>
#include <util/parallelToSequential.h>

#include <doctest.h>

TEST_CASE("parallel_to_sequential") {
    util::parallelToSequential(
        4,
        100,
        [](size_t workerId, size_t sequenceId) {
            LOG("Worker {} processing {}", workerId, sequenceId);
        },
        [](size_t sequenceId) {
            LOG("Sequentially processing {}", sequenceId);
        });
}

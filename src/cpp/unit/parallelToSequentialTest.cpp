#include <util/log.h>
#include <util/parallelToSequential.h>

#include <doctest.h>

#include <chrono>
#include <thread>

using namespace std::literals;

TEST_CASE("parallel_to_sequential") {
    util::parallelToSequential(
        std::thread::hardware_concurrency(),
        100,
        [](size_t workerId, size_t sequenceId) {
            LOG("Worker {:3} parallel     processing {:5} start", workerId, sequenceId);
            std::this_thread::sleep_for(1ms);
            LOG("Worker {:3} parallel     processing {:5} done", workerId, sequenceId);
        },
        [](size_t workerId, size_t sequenceId) {
            LOG("Worker {:3} sequentially processing {:5} start", workerId, sequenceId);
            std::this_thread::sleep_for(100ms);
            LOG("Worker {:3} sequentially processing {:5} done", workerId, sequenceId);
        });
}

#include <util/log.h>
#include <util/parallelToSequential.h>

#include <doctest.h>
#include <fmt/ostream.h>

#include <chrono>
#include <thread>

using namespace std::literals;

TEST_CASE("parallel_to_sequential") {
    static constexpr auto showLog = util::Log::show;
    // static constexpr auto showLog = util::Log::hide;

    static constexpr auto numItems = util::SequenceId{100};
    auto hasIdProcessed = std::vector<uint8_t>(numItems.count(), 0);

    auto expectedSequenceNumber = util::SequenceId{};
    util::parallelToSequential(
        numItems,
        util::ResourceId{5},
        util::ConcurrentWorkers{std::thread::hardware_concurrency()},
        [&](util::ResourceId resourceId, util::SequenceId sequenceId) {
            LOG_IF(showLog,
                   "{}: parallel resource {} start on {}",
                   std::this_thread::get_id(),
                   resourceId.count(),
                   sequenceId.count());
            REQUIRE(hasIdProcessed[sequenceId.count()] == 0);
            hasIdProcessed[sequenceId.count()] = 1;
            LOG_IF(showLog,
                   "{}: parallel resource {} stop on {}",
                   std::this_thread::get_id(),
                   resourceId.count(),
                   sequenceId.count());
        },
        [&](util::ResourceId resourceId, util::SequenceId sequenceId) {
            LOG_IF(showLog,
                   "{}: sequential resource {} start on {}",
                   std::this_thread::get_id(),
                   resourceId.count(),
                   sequenceId.count());
            REQUIRE(sequenceId == expectedSequenceNumber);
            REQUIRE(hasIdProcessed[sequenceId.count()] == 1);
            REQUIRE(++hasIdProcessed[sequenceId.count()]);
            ++expectedSequenceNumber;
            LOG_IF(showLog,
                   "{}: sequential resource {} stop on {}",
                   std::this_thread::get_id(),
                   resourceId.count(),
                   sequenceId.count());
        });

    for (auto b : hasIdProcessed) {
        REQUIRE(b == 2);
    }
}

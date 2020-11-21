#include "parallelToSequential.h"

#include <util/log.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

namespace {}

namespace util {

void parallelToSequential(size_t numWorkers,
                          size_t sequenceSize,
                          std::function<void(size_t, size_t)> const& parallelWorker,
                          std::function<void(size_t, size_t)> const& sequentialWorker) {

    auto nextSequentialSequenceId = size_t();
    auto finishedSequenceIds = std::unordered_set<size_t>();
    auto mutex = std::mutex();

    auto atomicSequenceId = std::atomic<size_t>(0);
    auto parallelWorkers = std::vector<std::thread>();

    for (auto i = size_t(); i < numWorkers; ++i) {
        parallelWorkers.emplace_back([&] {
            auto myWorkerId = i;
            auto mySequenceId = atomicSequenceId++;
            while (mySequenceId < sequenceSize) {
                parallelWorker(myWorkerId, mySequenceId);
                {
                    auto lock = std::scoped_lock(mutex);
                    finishedSequenceIds.insert(mySequenceId);
                    LOG("{:5} entries", finishedSequenceIds.size());

                    // try to process sequence IDs, if possible. TODO(martinus) This can be done a bit smarter. E.g. don't insert
                    // when we know it's us who can continue working.
                    while (1 == finishedSequenceIds.erase(nextSequentialSequenceId)) {
                        sequentialWorker(myWorkerId, nextSequentialSequenceId);
                        ++nextSequentialSequenceId;
                    }
                }

                mySequenceId = atomicSequenceId++;
            }
        });
    }

    for (auto& worker : parallelWorkers) {
        worker.join();
    }
}

} // namespace util

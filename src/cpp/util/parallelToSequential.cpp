#include "parallelToSequential.h"

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

    // nextSequentialSequenceId and finishedSequenceIds are protected by mutex
    auto nextSequentialSequenceId = size_t();
    auto finishedSequenceIds = std::unordered_set<size_t>();
    auto mutex = std::mutex();

    // atomic sequence id
    auto atomicSequenceId = std::atomic<size_t>(0);

    auto parallelWorkers = std::vector<std::thread>();
    for (auto i = size_t(); i < numWorkers; ++i) {
        parallelWorkers.emplace_back([&] {
            auto myWorkerId = i;
            while (true) {
                // get next sequenceId to work on
                auto mySequenceId = atomicSequenceId++;
                if (mySequenceId >= sequenceSize) {
                    // no valid work item any more => stop the worker.
                    break;
                }

                // do the parallel work
                parallelWorker(myWorkerId, mySequenceId);

                // now that parallel work has finished, put our sequence ID into the container
                auto lock = std::scoped_lock(mutex);
                if (mySequenceId == nextSequentialSequenceId) {
                    // sequential work can be done! process as many as we can
                    do {
                        sequentialWorker(myWorkerId, nextSequentialSequenceId++);
                    } while (1 == finishedSequenceIds.erase(nextSequentialSequenceId));
                } else {
                    // can't  process sequentially, put sequenceId into container for another worker
                    finishedSequenceIds.insert(mySequenceId);
                }
            }
        });
    }

    for (auto& worker : parallelWorkers) {
        worker.join();
    }
}

} // namespace util

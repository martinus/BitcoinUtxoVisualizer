#include "parallelToSequential.h"

#include <util/ConcurrentStack.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace std {

template <>
struct hash<util::SequenceId> {
    auto operator()(util::SequenceId const& sequenceId) const noexcept -> size_t {
        return std::hash<size_t>{}(sequenceId.count());
    }
};

} // namespace std

namespace util {

void parallelToSequential(SequenceId sequenceSize,
                          ResourceId numResources,
                          std::function<void(ResourceId, SequenceId)> const& parallelWorker,
                          std::function<void(ResourceId, SequenceId)> const& sequentialWorker) {
    parallelToSequential(
        sequenceSize, numResources, ConcurrentWorkers{std::thread::hardware_concurrency()}, parallelWorker, sequentialWorker);
}

// Some TODO's:
//
// * Sequential processing blocks parallel processing. Don't do this.
// * If sequential processing is slow, the container can grow unbounded. Limit growth by making workers wait.
// * Main thread should also do work, instead of just join().
void parallelToSequential(SequenceId sequenceSize,
                          ResourceId numResources,
                          ConcurrentWorkers numConcurrentWorkers,
                          std::function<void(ResourceId, SequenceId)> const& parallelWorker,
                          std::function<void(ResourceId, SequenceId)> const& sequentialWorker) {

    // fill up stack with all resourceIds
    auto availableResources = util::ConcurrentStack<ResourceId>();
    for (auto resourceId = ResourceId{}; resourceId != numResources; ++resourceId) {
        availableResources.push(resourceId);
    }

    // nextSequentialSequenceId and finishedSequenceIds are protected by mutex
    auto nextSequentialSequenceId = SequenceId();
    auto finishedSequenceToResource = std::unordered_map<SequenceId, ResourceId>();
    auto mutex = std::mutex();

    // atomic sequence id
    auto atomicSequenceId = std::atomic<size_t>(0);

    auto parallelWorkers = std::vector<std::thread>();
    for (auto w = ConcurrentWorkers(); w < numConcurrentWorkers; ++w) {
        parallelWorkers.emplace_back([&] {
            while (true) {
                // get a resource to work with. Blocks until one is available
                auto myResourceId = availableResources.pop();

                // Got a resource! now get a sequenceId to work with
                auto mySequenceId = SequenceId{atomicSequenceId++};
                if (mySequenceId >= sequenceSize) {
                    // no valid work item any more => put back the resource, and stop the worker.
                    availableResources.push(myResourceId);
                    break;
                }

                // do the parallel work
                parallelWorker(myResourceId, mySequenceId);

                // now that parallel work has finished, put our sequenceId and resourceId into the container for sequential
                // processing
                auto lock = std::scoped_lock(mutex);
                if (mySequenceId == nextSequentialSequenceId) {
                    // sequential work can be done! process as many as we can
                    while (true) {
                        sequentialWorker(myResourceId, nextSequentialSequenceId);
                        // work done! make the resource available immediately so ohter workers can continue
                        availableResources.push(myResourceId);

                        // let's see if the next sequentialId is avaialble
                        ++nextSequentialSequenceId;
                        auto it = finishedSequenceToResource.find(nextSequentialSequenceId);
                        if (it != finishedSequenceToResource.end()) {
                            myResourceId = it->second;
                            finishedSequenceToResource.erase(it);
                        } else {
                            // nextSequentialSequenceId has not finished yet
                            break;
                        }
                    }
                } else {
                    // can't  process sequentially, put sequenceId into container for another worker
                    finishedSequenceToResource.emplace(mySequenceId, myResourceId);
                }
            }
        });
    }

    for (auto& worker : parallelWorkers) {
        worker.join();
    }
}

} // namespace util

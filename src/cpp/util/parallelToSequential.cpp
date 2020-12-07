#include "parallelToSequential.h"

#include <util/ConcurrentStack.h>

#include <atomic>
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

// Two threadsafe queues: availableResources, and finishedParallelWork.
// * Parallel workers take a resourceId & next sequentialId, process, then put their resourceId & sequentialId result into finishedParallelWork.
// * Sequential worker takes from finishedParallelWork, and creates a map sequenceId -> resourceId.
//   If the next sequentialId is available, remove it from the map and perform sequential processing. Put resource back for the next parallel worker.
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
    auto finishedParallelWork = util::ConcurrentStack<std::pair<ResourceId, SequenceId>>();

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
                finishedParallelWork.push(myResourceId, mySequenceId);
            }
        });
    }

    // sequential worker takes items from finishedParalleWork and processes all it can.
    auto nextSequentialSequenceId = SequenceId();
    auto finishedSequenceToResource = std::unordered_map<SequenceId, ResourceId>();

    while (nextSequentialSequenceId < sequenceSize) {
        // fetch from finished parallel work
        auto [resourceId, sequenceId] = finishedParallelWork.pop();
        finishedSequenceToResource.emplace(sequenceId, resourceId);

        // process all the sequential work that can be done
        while (nextSequentialSequenceId < sequenceSize) {
            auto it = finishedSequenceToResource.find(nextSequentialSequenceId);
            if (it != finishedSequenceToResource.end()) {
                resourceId = it->second;
                finishedSequenceToResource.erase(it);
            } else {
                // nextSequentialSequenceId has not finished yet
                break;
            }

            // process the work, afterwards immediately make the resource available so other workers can continue
            sequentialWorker(resourceId, nextSequentialSequenceId);
            availableResources.push(resourceId);

            // let's see if the next sequentialId is avaialble
            ++nextSequentialSequenceId;
        }
    }

    // make sure all threads finish
    for (auto& worker : parallelWorkers) {
        worker.join();
    }
}

} // namespace util

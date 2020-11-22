#pragma once

#include <util/TypedNumber.h>

#include <cstddef>
#include <functional>

namespace util {

using SequenceId = util::TypedNumber<struct SequenceIdTag, size_t>;
using ResourceId = util::TypedNumber<struct ResourceIdTag, size_t>;
using ConcurrentWorkers = util::TypedNumber<struct ConcurrentWorkersTag, size_t>;

// parallel processing of a range, with N workers, sequential output.
//
// @param[in] sequenceSize Number of work items to process
// @param[in] numWorkers Number of parallel workers. Use e.g. std::thread::hardware_concurrency()
// @param[in] parallelWorker Worker that will be called in parallel, with workerNr, and sequenceNr.
// @param[in] sequencialWorker Worker that will be called with the increasing sequenceNr's after they were processed
void parallelToSequential(SequenceId sequenceSize,
                          ResourceId numResources,
                          ConcurrentWorkers numConcurrentWorkers,
                          std::function<void(ResourceId, SequenceId)> const& parallelWorker,
                          std::function<void(ResourceId, SequenceId)> const& sequentialWorker);

void parallelToSequential(SequenceId sequenceSize,
                          ResourceId numResources,
                          std::function<void(ResourceId, SequenceId)> const& parallelWorker,
                          std::function<void(ResourceId, SequenceId)> const& sequentialWorker);

} // namespace util

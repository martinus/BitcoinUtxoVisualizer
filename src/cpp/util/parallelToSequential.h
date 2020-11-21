#pragma once

#include <cstddef>
#include <functional>

namespace util {

// parallel processing of a range, with N workers, sequential output.
//
// @param[in] numWorkers Number of parallel workers. Use e.g. std::thread::hardware_concurrency()
// @param[in] sequenceSize Number of work items to process
// @param[in] parallelWorker Worker that will be called in parallel, with workerNr, and sequenceNr.
// @param[in] sequencialWorker Worker that will be called with the increasing sequenceNr's after they were processed
void parallelToSequential(size_t numWorkers,
                          size_t sequenceSize,
                          std::function<void(size_t, size_t)> const& parallelWorker,
                          std::function<void(size_t, size_t)> const& sequentialWorker);

} // namespace util

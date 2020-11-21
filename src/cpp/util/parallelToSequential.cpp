#include "parallelToSequential.h"

namespace util {

void parallelToSequential(size_t numWorkers,
                          size_t sequenceSize,
                          std::function<void(size_t, size_t)> const& parallelWorker,
                          std::function<void(size_t)> const &sequentialWorker) {
    for (size_t i = 0; i < sequenceSize; ++i) {
        parallelWorker(i % numWorkers, i);
        sequentialWorker(i);
    }
}

} // namespace util

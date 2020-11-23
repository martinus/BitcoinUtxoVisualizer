
#pragma once

#include <util/LogEnum.h>

#include <chrono>

namespace util {

class LogThrottler {
    std::chrono::nanoseconds mDelay{};
    std::chrono::steady_clock::time_point mNextDeadline{};

public:
    explicit LogThrottler(std::chrono::nanoseconds delay)
        : mDelay(delay) {}

    // Log::show if enough time has passed, otherwise Log::hide
    [[nodiscard]] auto operator()() -> Log {
        auto now = std::chrono::steady_clock::now();
        if (now >= mNextDeadline) {
            mNextDeadline = now + mDelay;
            return Log::show;
        }
        return Log::hide;
    }
};

} // namespace util

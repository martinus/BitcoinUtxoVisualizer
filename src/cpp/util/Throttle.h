
#pragma once

#include <chrono>

namespace util {

// (now/period + 1) * period
// period=10
// (7/10 + 1) * 10 = 10
// (9/10 + 1) * 10 = 10
// (10/10 + 1) * 10 = 20
// (19/10 + 1) * 10 = 20
// Allows one event per period.
class ThrottlePeriodic {
    std::chrono::nanoseconds mPeriodDuration{};
    std::chrono::steady_clock::time_point mNextAllowedPeriod{};

public:
    explicit ThrottlePeriodic(std::chrono::nanoseconds periodDuration)
        : mPeriodDuration(periodDuration) {}

    // Log::show if enough time has passed, otherwise Log::hide
    [[nodiscard]] auto operator()() -> bool {
        auto now = std::chrono::steady_clock::now();
        if (now >= mNextAllowedPeriod) {
            mNextAllowedPeriod =
                std::chrono::steady_clock::time_point((now.time_since_epoch() / mPeriodDuration + 1) * mPeriodDuration);
            return true;
        }
        return false;
    }
};

class ThrottleDelay {
    std::chrono::nanoseconds mDelay{};
    std::chrono::steady_clock::time_point mNextAllowedPeriod{};

public:
    explicit ThrottleDelay(std::chrono::nanoseconds delay)
        : mDelay(delay) {}

    // Log::show if enough time has passed, otherwise Log::hide
    [[nodiscard]] auto operator()() -> bool {
        auto now = std::chrono::steady_clock::now();
        if (now >= mNextAllowedPeriod) {
            mNextAllowedPeriod = now + mDelay;
            return true;
        }
        return false;
    }
};

} // namespace util

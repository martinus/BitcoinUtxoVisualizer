#pragma once

#include <util/LogEnum.h>

#include <fmt/core.h>

#include <chrono>
#include <mutex>
#include <string_view>
#include <thread>

namespace util {

static std::chrono::steady_clock::time_point sStartTime = std::chrono::steady_clock::now();

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG(...)                                      \
    do {                                              \
        ::util::log(__FILE__, __LINE__, __VA_ARGS__); \
    } while (0)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_IF(showHide, ...)                             \
    do {                                                  \
        if (::util::Log::show == (showHide)) {            \
            ::util::log(__FILE__, __LINE__, __VA_ARGS__); \
        }                                                 \
    } while (0)

// Simple logging with a global lock
template <typename... Args>
void log(std::string_view filename, int line, char const* format, Args&&... args) {
    // sync not really needed when only printing lines
    static std::mutex sLogMutex{};

    if (filename.size() > 30) {
        filename = filename.substr(filename.size() - 30);
    }
    auto sec = std::chrono::duration<double>{std::chrono::steady_clock::now() - sStartTime};

    auto guard = std::lock_guard<std::mutex>{sLogMutex};
    fmt::print("{:8.3f} {:>30}({:3}) | ", sec.count(), filename, line);
    fmt::print(format, std::forward<Args>(args)...);
    fmt::print("\n");
    fflush(stdout);
}

} // namespace util

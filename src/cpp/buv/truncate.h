#pragma once

namespace buv {

template <typename T>
static auto truncate(T min, T val, T max) -> T {
    if (val <= min) {
        return min;
    }
    if (val >= max) {
        return max;
    }
    return val;
}

template <typename T>
static auto truncate(T val, T max) -> T {
    return val >= max ? max : val;
}

} // namespace buv

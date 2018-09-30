#pragma once

namespace bv {

template <typename T>
static T truncate(T min, double val, T max)
{
    if (val <= min) {
        return min;
    }
    if (val >= max) {
        return max;
    }
    return static_cast<T>(val);
}

} // namespace bv
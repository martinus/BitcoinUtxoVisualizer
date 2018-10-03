#pragma once

namespace bv {

template <typename T>
static T truncate(T min, T val, T max)
{
    if (val <= min) {
        return min;
    }
    if (val >= max) {
        return max;
    }
    return val;
}

template <typename T>
static T truncate(T val, T max)
{
    return val >= max ? max : val;
}

} // namespace bv
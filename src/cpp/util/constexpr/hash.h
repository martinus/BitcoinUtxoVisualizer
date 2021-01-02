#pragma once

#include <cstdint>
#include <string_view>

namespace util::cx {

namespace detail {

[[nodiscard]] constexpr auto mix(uint64_t h) -> uint64_t {
    h ^= h >> 23U;
    h *= UINT64_C(0x2127599bf4325c37);
    h ^= h >> 47U;
    return h;
}

[[nodiscard]] constexpr auto block2(char const* str) -> uint64_t {
    return static_cast<uint64_t>(str[1]) | (static_cast<uint64_t>(str[0]) << 8U);
}

[[nodiscard]] constexpr auto block4(char const* str) -> uint64_t {
    return block2(str + 2) | (block2(str) << 16U);
}

[[nodiscard]] constexpr auto block8(char const* str) -> uint64_t {
    return block4(str + 4) | (block4(str) << 32U);
}

} // namespace detail

[[nodiscard]] constexpr auto defaultSeed() -> uint64_t {
    return 12345;
}

// Currently implements fasthash64 (see https://github.com/ztanml/fast-hash/blob/master/fasthash.h)
[[nodiscard]] constexpr auto hash(char const* str, size_t len, uint64_t seed) -> uint64_t {
    constexpr auto m = UINT64_C(0x880355f21e6d1965);

    auto const* end = str + (len / 8) * 8;
    auto h = seed ^ (len * m);

    while (str != end) {
        auto v = detail::block8(str);
        str += 8;
        h ^= detail::mix(v);
        h *= m;
    }

    auto v = uint64_t();

    switch (len & 7U) {
    case 7:
        v ^= static_cast<uint64_t>(str[6]) << 48U;
    case 6:
        v ^= static_cast<uint64_t>(str[5]) << 40U;
    case 5:
        v ^= static_cast<uint64_t>(str[4]) << 32U;
    case 4:
        v ^= static_cast<uint64_t>(str[3]) << 24U;
    case 3:
        v ^= static_cast<uint64_t>(str[2]) << 16U;
    case 2:
        v ^= static_cast<uint64_t>(str[1]) << 8U;
    case 1:
        v ^= static_cast<uint64_t>(str[0]);
        h ^= detail::mix(v);
        h *= m;
    }

    return detail::mix(h);
}

[[nodiscard]] constexpr auto hash(char const* str, size_t len) -> uint64_t {
    return hash(str, len, defaultSeed());
}

[[nodiscard]] constexpr auto hash(std::string_view sv, uint64_t seed) -> uint64_t {
    return hash(sv.data(), sv.size(), seed);
}

[[nodiscard]] constexpr auto hash(std::string_view sv) -> uint64_t {
    return hash(sv, defaultSeed());
}

[[nodiscard]] constexpr auto operator""_hash(char const* str, size_t s) -> uint64_t {
    return hash(str, s);
}

} // namespace util::cx

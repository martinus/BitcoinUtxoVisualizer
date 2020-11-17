#pragma once

#include <robin_hood.h>

#include <array>
#include <cstdint>
#include <cstring> // std::memcpy

namespace buv {

// 128 bit should be enough to prevent any collisions
using TxId = std::array<uint8_t, 16>;

// maximum 65k outputs should be plenty (current record is ~13k)
using VOutNr = uint16_t;

// using array of bytes instead of uint64_t so we don't have any alignment problems. Must use std::memcpy to get the data out
// without any undefined behavior.
class Satoshi final {
    static constexpr auto NumBytes = sizeof(uint64_t);
    std::array<uint8_t, NumBytes> mData{};

public:
    constexpr Satoshi() noexcept = default;

    inline explicit Satoshi(uint64_t x) noexcept {
        std::memcpy(mData.data(), &x, NumBytes);
    }

    inline auto value() noexcept -> uint64_t {
        auto x = uint64_t();
        std::memcpy(&x, mData.data(), NumBytes);
        return x;
    }
};

using Utxo = robin_hood::unordered_flat_map<TxId, robin_hood::unordered_flat_map<VOutNr, Satoshi>>;

} // namespace buv

namespace robin_hood {

template <>
struct hash<buv::TxId> {
    auto operator()(buv::TxId const& txid) const noexcept -> size_t {
        auto h = size_t();
        std::memcpy(&h, txid.data(), sizeof(size_t));
        return h;
    }
};

} // namespace robin_hood
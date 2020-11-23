#pragma once

#include <robin_hood.h>

#include <array>
#include <cstdint>
#include <cstring> // std::memcpy
#include <filesystem>
#include <string_view>
#include <unordered_map>

namespace buv {

// 128 bit should be enough to prevent any collisions
using TxId = std::array<uint8_t, 16>;

// maximum 65k outputs should be plenty (current record is ~13k)
using VOutNr = uint16_t;

// using array of bytes instead of int64_t so we don't have any alignment problems. Must use std::memcpy to get the data out
// without any undefined behavior.
class Satoshi final {
    static constexpr auto NumBytes = sizeof(int64_t);
    std::array<uint8_t, NumBytes> mData{};

public:
    constexpr Satoshi() noexcept = default;

    inline explicit Satoshi(int64_t x) noexcept {
        std::memcpy(mData.data(), &x, NumBytes);
    }

    [[nodiscard]] inline auto value() const noexcept -> int64_t {
        auto x = int64_t();
        std::memcpy(&x, mData.data(), NumBytes);
        return x;
    }
};

// special entry with VOutNr==65535 is blocksize
using UtxoPerTx = robin_hood::unordered_flat_map<VOutNr, Satoshi>;
// using UtxoPerTx = std::unordered_map<VOutNr, Satoshi>;

using Utxo = robin_hood::unordered_node_map<TxId, UtxoPerTx>;
// using Utxo = std::unordered_map<TxId, UtxoPerTx>;

auto loadUtxo(std::filesystem::path const& utxoFilename) -> Utxo;
void storeUtxo(Utxo const& utxo, std::filesystem::path const& utxoFilename);

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

namespace std {

template <>
struct hash<buv::TxId> {
    auto operator()(buv::TxId const& txid) const noexcept -> size_t {
        auto h = size_t();
        std::memcpy(&h, txid.data(), sizeof(size_t));
        return h;
    }
};

} // namespace std
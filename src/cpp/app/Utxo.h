#pragma once

#include <robin_hood.h>

#include <array>
#include <cstdint>
#include <cstring> // std::memcpy
#include <filesystem>
#include <map>
#include <string_view>
#include <unordered_map>

namespace buv {

// Limiting to 8 bytes for now. There is a slim risk of collision, but otherwise it takes sooo much memory.
// Probability of a collision is approximately (see
// https://math.stackexchange.com/questions/883983/birthday-paradox-huge-numbers):
//   mapsize^2  / (2 * 2^64)
// So for 50M entries, the probability is 0.0068%.
static constexpr auto txidPrefixSize = size_t(8);
using TxIdPrefix = std::array<uint8_t, 8>;

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

// using UtxoPerTx = std::unordered_map<VOutNr, Satoshi>;
using UtxoPerTx = robin_hood::unordered_flat_map<VOutNr, Satoshi>;
// using UtxoPerTx = std::map<VOutNr, Satoshi>;

using Utxo = robin_hood::unordered_node_map<TxIdPrefix, UtxoPerTx>;
// using Utxo = std::unordered_map<TxId, UtxoPerTx>;
// using Utxo = std::map<TxIdPrefix, UtxoPerTx>;

auto loadUtxo(std::filesystem::path const& utxoFilename) -> std::unique_ptr<Utxo>;
void storeUtxo(Utxo const& utxo, std::filesystem::path const& utxoFilename);

} // namespace buv

namespace robin_hood {

template <>
struct hash<buv::TxIdPrefix> {
    auto operator()(buv::TxIdPrefix const& txid) const noexcept -> size_t {
        auto h = size_t();
        std::memcpy(&h, txid.data(), sizeof(size_t));
        return h;
    }
};

} // namespace robin_hood

namespace std {

template <>
struct hash<buv::TxIdPrefix> {
    auto operator()(buv::TxIdPrefix const& txid) const noexcept -> size_t {
        auto h = size_t();
        std::memcpy(&h, txid.data(), sizeof(size_t));
        return h;
    }
};

} // namespace std

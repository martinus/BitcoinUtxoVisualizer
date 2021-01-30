#pragma once

#include <app/Chunk.h>
#include <util/log.h>

#include <fmt/core.h>
#include <robin_hood.h>

#include <filesystem>

namespace buv {

// Limiting to 8 bytes for now. There is a slim risk of collision, but otherwise it takes sooo much memory.
// Probability of a collision is approximately (see
// https://math.stackexchange.com/questions/883983/birthday-paradox-huge-numbers):
//   mapsize^2  / (2 * 2^64)
// So for 50M entries, the probability is 0.0068%.
static constexpr auto txidPrefixSize = size_t(8);
using TxIdPrefix = std::array<uint8_t, 8>;

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

namespace buv {

// try to do small-utxo optimization: directly encode 2 vout's here.
// Memory layout:
// 8 byte: VoutSatoshi::emptyMask: not using small utxo optimization. vout == 1: place has a satoshi value. vout==0: place has NO
// satoshi value 8 byte: (vout==1: place has a satoshi value, vout==0: place has NO satoshi value) OR pointer to chunk 4 byte:
// blockheight
class UtxoPerTx {
    static_assert(sizeof(void*) == sizeof(VoutSatoshi));

    // use an array so we we can get no padding
    std::array<uint8_t, sizeof(VoutSatoshi) * 2> mChunkPtrOrVoutSatoshi{};
    uint32_t mBlockHeight = 0;

public:
    // sets the satoshi. Returns true if smallUtxoOptimization is used.
    auto satoshi(ChunkStore& chunkStore, std::vector<int64_t> const& sat) -> bool {
        if (sat.size() <= 2) {
            // put into small opt
            for (size_t i = 0; i < sat.size(); ++i) {
                voutSatoshi(static_cast<uint16_t>(i), sat[i]);
            }
            return true;
        }

        // put all into chunks
        auto* ptr = chunkStore.insert(0, sat[0], nullptr);
        chunk(ptr);
        for (size_t i = 1; i < sat.size(); ++i) {
            ptr = chunkStore.insert(i, sat[i], ptr);
        }
        return false;
    }

    [[nodiscard]] auto chunk() const -> Chunk* {
        Chunk* ptr = nullptr;
        std::memcpy(&ptr, mChunkPtrOrVoutSatoshi.data() + sizeof(void*), sizeof(void*));
        return ptr;
    }

    void chunk(Chunk* ptr) {
        // mark as !isSmallUtxo()
        auto emptyVoutSatoshi = VoutSatoshi();
        std::memcpy(mChunkPtrOrVoutSatoshi.data(), &emptyVoutSatoshi, sizeof(void*));

        std::memcpy(mChunkPtrOrVoutSatoshi.data() + sizeof(void*), &ptr, sizeof(void*));
    }

    void voutSatoshi(uint16_t idx, int64_t satoshi) {
        // vout==1 means place is taken
        auto vs = VoutSatoshi(1, satoshi);
        std::memcpy(mChunkPtrOrVoutSatoshi.data() + sizeof(VoutSatoshi) * idx, &vs, sizeof(VoutSatoshi));
    }

    [[nodiscard]] auto blockHeight() const -> uint32_t {
        return mBlockHeight;
    }

    void blockHeight(uint32_t bh) {
        mBlockHeight = bh;
    }

    [[nodiscard]] auto isSmallUtxo() const -> bool {
        auto vs = VoutSatoshi();
        std::memcpy(&vs, mChunkPtrOrVoutSatoshi.data(), sizeof(void*));
        return !vs.isEmptyMask();
    }

    [[nodiscard]] auto removeVoutSatoshi(size_t idx) -> int64_t {
        // get the value
        auto voutSatoshi = VoutSatoshi();
        std::memcpy(&voutSatoshi, mChunkPtrOrVoutSatoshi.data() + sizeof(VoutSatoshi) * idx, sizeof(VoutSatoshi));
        auto sat = voutSatoshi.satoshi();

        // reset the value to 0
        voutSatoshi = VoutSatoshi{0, 0};
        std::memcpy(mChunkPtrOrVoutSatoshi.data() + sizeof(VoutSatoshi) * idx, &voutSatoshi, sizeof(VoutSatoshi));

        return sat;
    }

    [[nodiscard]] auto empty() const -> bool {
        if (isSmallUtxo()) {
            auto x1 = uint64_t();
            std::memcpy(&x1, mChunkPtrOrVoutSatoshi.data(), sizeof(VoutSatoshi));
            auto x2 = uint64_t();
            std::memcpy(&x2, mChunkPtrOrVoutSatoshi.data() + sizeof(VoutSatoshi), sizeof(VoutSatoshi));
            return x1 == 0U && x2 == 0U;
        }
        return chunk() == nullptr;
    }
};

static_assert(sizeof(UtxoPerTx) == 8 + 8 + 4);

class Utxo {
    ChunkStore mChunkStore{};
    using Map = robin_hood::unordered_node_map<TxIdPrefix, UtxoPerTx>;
    // using Map = std::unordered_map<TxIdPrefix, UtxoPerTx>;
    Map mTxidToUtxos{};

    static_assert(sizeof(Map::value_type) == sizeof(TxIdPrefix) + sizeof(UtxoPerTx));

public:
    Utxo() {
        // we certainly have to keep a lot of data around. Reserve because we know we'll need it
        mTxidToUtxos.reserve(100'000'000);
    }

    template <typename Op>
    void removeAllSorted(TxIdPrefix const& txIdPrefix, std::vector<uint16_t> const& vouts, Op&& op) {
        if (auto it = mTxidToUtxos.find(txIdPrefix); it != mTxidToUtxos.end()) {
            auto& utxoPerTx = it->second;
            auto blockHeight = utxoPerTx.blockHeight();

            if (utxoPerTx.isSmallUtxo()) {
                // small utxo optimization: does not change the values for now. If we'd do that, the isSmallUtxo() detection fails
                // TODO(martinus) we need to figure out when it's empty! so we can remove the entry.
                for (auto vout : vouts) {
                    op(utxoPerTx.removeVoutSatoshi(vout), blockHeight);
                }
                if (utxoPerTx.empty()) {
                    mTxidToUtxos.erase(it);
                }
            } else {
                auto* oldRoot = utxoPerTx.chunk();

                auto newRoot = mChunkStore.removeAllSorted(vouts, oldRoot, [blockHeight, &op](int64_t satoshi) {
                    op(satoshi, blockHeight);
                });
                if (newRoot == nullptr) {
                    // whole transaction was consumed, remove it from the map
                    mTxidToUtxos.erase(it);
                } else if (newRoot != oldRoot) {
                    utxoPerTx.chunk(newRoot);
                }
            }
        } else {
            throw std::runtime_error("DAMN! did not find txid");
        }
    }

    // Creates an entry in the table, and returns an Inserter where the vout's can be inserted.
    // returns true if small UTXO optimization is used
    auto insert(TxIdPrefix const& txIdPrefix, uint32_t blockHeight, std::vector<int64_t> const& satoshi) -> bool {
        auto& utxoPerTx = mTxidToUtxos[txIdPrefix];
        utxoPerTx.blockHeight(blockHeight);
        return utxoPerTx.satoshi(mChunkStore, satoshi);
    }

    [[nodiscard]] auto map() const -> Map const& {
        return mTxidToUtxos;
    }

    [[nodiscard]] auto chunkStore() const -> ChunkStore const& {
        return mChunkStore;
    }
};

// Compact binary data serialization
void serialize(uint32_t blockHeight, Utxo const& utxo, std::filesystem::path const& filename);

[[nodiscard]] auto load(std::filesystem::path const& filename) -> std::pair<uint32_t, Utxo>;

} // namespace buv

#if 0

template <>
class fmt::formatter<buv::Utxo> {
    bool mIsDetailed = false;

public:
    auto parse(fmt::format_parse_context& ctx) -> format_parse_context::iterator;
    auto format(buv::Utxo const& utxo, format_context& ctx) const -> format_context::iterator;
};

#endif
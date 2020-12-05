#pragma once

#include <app/Chunk.h>
#include <util/log.h>

#include <fmt/core.h>
#include <robin_hood.h>

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

class UtxoPerTx {
    // use an array so we we can get no padding
    std::array<uint8_t, sizeof(void*)> mChunkPtr{};
    uint32_t mBlockHeight = 0;

public:
    [[nodiscard]] auto chunk() const -> Chunk* {
        Chunk* ptr = nullptr;
        std::memcpy(&ptr, mChunkPtr.data(), sizeof(void*));
        return ptr;
    }

    void chunk(Chunk* ptr) {
        std::memcpy(mChunkPtr.data(), &ptr, sizeof(void*));
    }

    [[nodiscard]] auto blockHeight() const -> uint32_t {
        return mBlockHeight;
    }

    void blockHeight(uint32_t bh) {
        mBlockHeight = bh;
    }
};

static_assert(sizeof(UtxoPerTx) == 8 + 4);

class VoutInserter {
    ChunkStore* mChunkStore = nullptr;
    UtxoPerTx* mUtxoPerTx = nullptr;
    Chunk* mLastChunk = nullptr;

public:
    VoutInserter(ChunkStore& chunkStore, UtxoPerTx& utxoPerTx)
        : mChunkStore(&chunkStore)
        , mUtxoPerTx(&utxoPerTx) {}

    void insert(uint16_t vout, int64_t satoshi) {
        mLastChunk = mChunkStore->insert(vout, satoshi, mLastChunk);
        if (mUtxoPerTx->chunk() == nullptr) {
            mUtxoPerTx->chunk(mLastChunk);
        }
    }
};

class Utxo {
    ChunkStore mChunkStore{};
    using Map = robin_hood::unordered_node_map<TxIdPrefix, UtxoPerTx>;
    // using Map = std::unordered_map<TxIdPrefix, UtxoPerTx>;
    Map mTxidToUtxos{};

    static_assert(sizeof(Map::value_type) == 8 + 8 + 4);

public:
    // Removes the utxo, and returns the amount & blockheight when it was added.
    [[nodiscard]] auto remove(TxIdPrefix const& txIdPrefix, uint16_t vout) -> std::pair<int64_t, uint32_t> {
        if (auto it = mTxidToUtxos.find(txIdPrefix); it != mTxidToUtxos.end()) {
            auto [satoshi, newChunk] = mChunkStore.remove(vout, it->second.chunk());
            auto blockHeight = it->second.blockHeight();
            if (newChunk == nullptr) {
                // whole transaction was consumed, remove it from the map
                mTxidToUtxos.erase(it);
            } else {
                if (newChunk->empty()) {
                    throw std::runtime_error("shouldn't be empty!");
                }
            }
            return std::make_pair(satoshi, blockHeight);
        }
        throw std::runtime_error("DAMN! did not find txid");
    }

    // Creates an entry in the table, and returns an Inserter where the vout's can be inserted.
    auto inserter(TxIdPrefix const& txIdPrefix, uint32_t blockHeight) -> VoutInserter {
        auto& utxoPerTx = mTxidToUtxos[txIdPrefix];
        utxoPerTx.blockHeight(blockHeight);
        return VoutInserter(mChunkStore, utxoPerTx);
    }

    [[nodiscard]] auto map() const -> Map const& {
        return mTxidToUtxos;
    }

    [[nodiscard]] auto chunkStore() const -> ChunkStore const& {
        return mChunkStore;
    }
};

} // namespace buv

template <>
class fmt::formatter<buv::Utxo> {
    bool mIsDetailed = false;

public:
    auto parse(fmt::format_parse_context& ctx) -> format_parse_context::iterator;
    auto format(buv::Utxo const& utxo, format_context& ctx) const -> format_context::iterator;
};

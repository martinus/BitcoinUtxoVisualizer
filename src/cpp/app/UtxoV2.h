#pragma once

#include <app/Chunk.h>

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

namespace buv {

struct UtxoPerTx {
    Chunk* chunk = nullptr;
    uint32_t blockHeight = 0;
};

class VoutInserter {
    ChunkStore* mChunkStore = nullptr;
    Chunk** mChunk = nullptr;
    Chunk* mLastChunk = nullptr;

public:
    VoutInserter(ChunkStore& chunkStore, Chunk*& chunk)
        : mChunkStore(&chunkStore)
        , mChunk(&chunk) {}

    void insert(uint16_t vout, int64_t satoshi) {
        mLastChunk = mChunkStore->insert(vout, satoshi, mLastChunk);
        if (*mChunk == nullptr) {
            *mChunk = mLastChunk;
        }
    }
};

class Utxo {
    ChunkStore mChunkStore{};
    robin_hood::unordered_node_map<TxIdPrefix, UtxoPerTx> mTxidToUtxos;

public:
    // Removes the utxo, and returns the amount & blockheight when it was added.
    [[nodiscard]] auto remove(TxIdPrefix const& txIdPrefix, uint16_t vout) -> std::pair<int64_t, uint32_t> {
        if (auto it = mTxidToUtxos.find(txIdPrefix); it != mTxidToUtxos.end()) {
            auto [satoshi, newChunk] = mChunkStore.remove(vout, it->second.chunk);
            auto blockHeight = it->second.blockHeight;
            if (newChunk == nullptr) {
                // whole transaction was consumed, remove it from the map
                mTxidToUtxos.erase(it);
            }
            return std::make_pair(satoshi, blockHeight);
        }
        throw std::runtime_error("DAMN! did not find txid");
    }

    // Creates an entry in the table, and returns an Inserter where the vout's can be inserted.
    auto inserter(TxIdPrefix const& txIdPrefix, uint32_t blockHeight) -> VoutInserter {
        auto& utxoPerTx = mTxidToUtxos[txIdPrefix];
        utxoPerTx.blockHeight = blockHeight;
        return VoutInserter(mChunkStore, utxoPerTx.chunk);
    }

    [[nodiscard]] auto map() const -> robin_hood::unordered_node_map<TxIdPrefix, UtxoPerTx> const& {
        return mTxidToUtxos;
    }

    [[nodiscard]] auto chunkStore() const -> ChunkStore const& {
        return mChunkStore;
    }
};

} // namespace buv

template <>
struct fmt::formatter<buv::Utxo> {
    static auto parse(fmt::format_parse_context& ctx) -> format_parse_context::iterator;
    static auto format(buv::Utxo const& utxo, format_context& ctx) -> format_context::iterator;
};

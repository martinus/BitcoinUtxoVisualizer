#include "UtxoV2.h"

#include <fmt/format.h>
#include <robin_hood.h>

#include <map>
#include <string>
#include <string_view>

namespace fmt {

auto formatter<buv::Utxo>::parse(format_parse_context& ctx) -> format_parse_context::iterator {
    const auto* it = ctx.begin();
    if (it == ctx.end() || *it == '}') {
        return it;
    }

    if (*it == 'd') {
        mIsDetailed = true;
        ++it;
    }

    if (it != ctx.end() && *it != '}') {
        throw format_error("invalid format");
    }

    return it;
}

auto formatter<buv::Utxo>::format(buv::Utxo const& utxo, format_context& ctx) const -> format_context::iterator {
    auto out = ctx.out();
    auto const& cs = utxo.chunkStore();

    format_to(out,
              "({} txids, {} vout's used, {} allocated ({} bulk))",
              utxo.map().size(),
              (cs.numAllocatedChunks() - cs.numFreeChunks()) * buv::Chunk::numVoutSatoshiPerChunk(),
              cs.numAllocatedChunks() * buv::Chunk::numVoutSatoshiPerChunk(),
              cs.numAllocatedBulks());

    if (mIsDetailed) {
        auto numvoutsToCount = robin_hood::unordered_flat_map<size_t, size_t>();
        for (auto const& kv : utxo.map()) {
            auto const* chunk = kv.second.chunk;
            do {
                ++numvoutsToCount[chunk->size()];
                chunk = chunk->next();
            } while (chunk != nullptr);
        }

        auto numvoutsAndCounts = std::vector<robin_hood::pair<size_t, size_t>>(numvoutsToCount.begin(), numvoutsToCount.end());

        // sort by number of counts
        std::sort(numvoutsAndCounts.begin(), numvoutsAndCounts.end(), [](auto const& a, auto const& b) {
            return a.second < b.second;
        });

        for (auto const& [vouts, count] : numvoutsAndCounts) {
            format_to(out, "\n\t{:10} x {:10} vouts", count, vouts);
        }
    }
    return out;
}

} // namespace fmt

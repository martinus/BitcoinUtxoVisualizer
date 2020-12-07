#include "Utxo.h"

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
              "({:10} txids, {:10} vout's used, {:10} allocated ({:4} bulk))",
              utxo.map().size(),
              cs.numAllocatedChunks() - cs.numFreeChunks(),
              cs.numAllocatedChunks(),
              cs.numAllocatedBulks());

    if (mIsDetailed) {
        // list counts 1-20
        static constexpr auto maxLen = size_t(20);
        auto numvoutsAndCounts = std::vector<std::pair<size_t, size_t>>(maxLen);
        for (size_t i = 0; i < maxLen; ++i) {
            numvoutsAndCounts[i].first = i + 1;
            numvoutsAndCounts[i].second = 0;
        }
        for (auto const& kv : utxo.map()) {
            auto const* chunk = kv.second.chunk();
            auto len = size_t();
            do {
                ++len;
                chunk = chunk->next();
            } while (chunk != nullptr && len < maxLen);
            ++numvoutsAndCounts[len - 1].second;
        }

        // sort by number of counts
        std::sort(numvoutsAndCounts.begin(), numvoutsAndCounts.end(), [](auto const& a, auto const& b) {
            return a.second < b.second;
        });

        for (auto const& [vouts, count] : numvoutsAndCounts) {
            format_to(out, "\n\t{:10} x {:4} {}", count, vouts, vouts == maxLen ? "or more vouts" : "vouts");
        }
    }
    return out;
}

} // namespace fmt

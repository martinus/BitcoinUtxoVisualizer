#include "UtxoV2.h"

#include <fmt/format.h>

#include <string>
#include <string_view>

namespace fmt {

auto formatter<buv::Utxo>::parse(format_parse_context& ctx) -> format_parse_context::iterator {
    const auto* it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
        throw format_error("invalid format");
    }
    return it;
}

auto formatter<buv::Utxo>::format(buv::Utxo const& utxo, format_context& ctx) -> format_context::iterator {
    auto out = ctx.out();
    auto const& cs = utxo.chunkStore();
    format_to(out,
              "({} txids, {} vout's used, {} allocated ({} bulk))",
              utxo.map().size(),
              (cs.numAllocatedChunks() - cs.numFreeChunks()) * buv::Chunk::numVoutSatoshiPerChunk(),
              cs.numAllocatedChunks() * buv::Chunk::numVoutSatoshiPerChunk(),
              cs.numAllocatedBulks());
    return out;
}

} // namespace fmt

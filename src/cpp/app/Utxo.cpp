#include "Utxo.h"

#include <util/writeBinary.h>

#include <fmt/format.h>
#include <robin_hood.h>

#include <fstream>
#include <map>
#include <string>
#include <string_view>

namespace buv {

namespace {

// first creates a .tmp file, then renames when finished.
auto dump(uint32_t blockHeight, Utxo const& utxo, std::filesystem::path const& filename) -> size_t {
    auto fout = std::ofstream(filename, std::ios::binary);
    if (!fout.is_open()) {
        throw std::runtime_error("could not open file for writing UTXO");
    }

    fout.write("UTXO0", 4);
    util::writeBinary<4>(blockHeight, fout);
    util::writeBinary<8>(utxo.map().size(), fout);
    auto numVouts = size_t();
    for (auto const& kv : utxo.map()) {
        // key
        util::writeArray<8>(kv.first, fout);

        // value
        auto const* chunk = kv.second.chunk();
        while (chunk != nullptr) {
            util::writeBinary<8>(chunk->voutSatoshi().data(), fout);
            chunk = chunk->next();
            ++numVouts;
        }
        util::writeBinary<8>(VoutSatoshi().data(), fout);
    }

    return numVouts;
}

} // namespace

// first creates a .tmp file, then renames when finished.
void serialize(uint32_t blockHeight, Utxo const& utxo, std::filesystem::path const& filename) {
    auto tmpFilename = filename;
    tmpFilename += ".tmp";
    LOG("Writing UTXO to {}...", tmpFilename.string());
    auto n = dump(blockHeight, utxo, tmpFilename.string());
    LOG("Wrote {} vouts", n);
    std::filesystem::rename(tmpFilename.string(), filename.string());
    LOG("Renamed {} -> {}", tmpFilename.string(), filename.string());
}

[[nodiscard]] auto load(std::filesystem::path const& filename) -> std::pair<uint32_t, Utxo> {
    auto fin = std::ifstream(filename, std::ios::binary);
    if (!fin.is_open()) {
        throw std::runtime_error("could not open file for reading UTXO");
    }

    auto header = util::readBinary<uint32_t>(fin);
    if (header != 0x1) {
        throw std::runtime_error(fmt::format("Got {:x} but expected {:x}", header, 0x1));
    }

    auto blockHeight = util::readBinary<uint32_t>(fin);

    auto utxo = Utxo();
    return std::make_pair(blockHeight, std::move(utxo));
}

} // namespace buv

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

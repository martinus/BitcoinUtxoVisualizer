#include <app/BlockEncoder.h>
#include <util/LogThrottler.h>
#include <util/Mmap.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/format.h>

using namespace std::literals;

TEST_CASE("decode_change" * doctest::skip()) {
    static constexpr auto fileName = "../../out/blocks/changes.blk1";
    auto mmapedFile = util::Mmap(fileName);
    if (!mmapedFile.is_open()) {
        throw std::runtime_error(fmt::format("could not open file {}", fileName));
    }

    auto throttler = util::LogThrottler(200ms);

    const auto* ptr = mmapedFile.begin();
    auto cib = buv::ChangesInBlock();
    auto expectedBlockHeight = uint32_t();
    auto totalChanges = size_t();
    while (ptr != mmapedFile.end()) {
        std::tie(cib, ptr) = buv::ChangesInBlock::decode(std::move(cib), ptr);
        LOG_IF(throttler(), "block {}, {} changes", cib.blockHeight(), cib.changeAtBlockheights().size());
        REQUIRE(cib.blockHeight() == expectedBlockHeight);
        ++expectedBlockHeight;

        totalChanges += cib.changeAtBlockheights().size();

        // at least a single change should occur, because coinbase
        REQUIRE(!cib.changeAtBlockheights().empty());
    }
    LOG("finished! last block: {}. Total changes: {}", cib.blockHeight(), totalChanges);
}

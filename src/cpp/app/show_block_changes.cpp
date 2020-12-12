#include <app/BlockEncoder.h>
#include <util/Mmap.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/core.h>

#include <filesystem>

TEST_CASE("show_block_changes" * doctest::skip()) {
    auto targetBlockHeight = uint32_t(489849);

    auto mmapedFile = util::Mmap("../../out/blocks/changes.blk1");
    if (!mmapedFile.is_open()) {
        throw std::runtime_error("could not open");
    }

    // skip forward
    auto currentBlockHeight = uint32_t(0);
    auto const* ptr = mmapedFile.begin();
    while (currentBlockHeight != targetBlockHeight) {
        std::tie(currentBlockHeight, ptr) = buv::ChangesInBlock::skip(ptr);
    }

    auto [cib, nextPtr] = buv::ChangesInBlock::decode(ptr);

    auto sum = int64_t(0);
    LOG("block {}:", cib.blockData().blockHeight);
    for (auto const& change : cib.changeAtBlockheights()) {
        sum += std::abs(change.satoshi());
        LOG("\t{:15.8f} BTC from {}", change.satoshi() / 100'000'000.0, change.blockHeight());
    }
    LOG("\t{:15.8f} BTC total changed", sum / 100'000'000.0);
}

#include <app/BlockEncoder.h>
#include <util/LogThrottler.h>
#include <util/Mmap.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

using namespace std::literals;

TEST_CASE("decode_change" * doctest::skip()) {
    auto before = std::chrono::steady_clock::now();

    static constexpr auto fileName = "../../out/blocks/changes.blk1";
    auto mmapedFile = util::Mmap(fileName);
    if (!mmapedFile.is_open()) {
        throw std::runtime_error(fmt::format("could not open file {}", fileName));
    }

    auto throttler = util::LogThrottler(1000ms);

    const auto* ptr = mmapedFile.begin();
    auto cib = buv::ChangesInBlock();
    auto expectedBlockHeight = uint32_t();
    auto totalChanges = size_t();
    while (ptr != mmapedFile.end()) {
        std::tie(cib, ptr) = buv::ChangesInBlock::decode(std::move(cib), ptr);
        LOG_IF(throttler(),
               "{:4}% block {}, {} changes",
               100 * std::distance(mmapedFile.begin(), ptr) / mmapedFile.size(),
               cib.blockHeight(),
               cib.changeAtBlockheights().size());
        REQUIRE(cib.blockHeight() == expectedBlockHeight);
        ++expectedBlockHeight;

        totalChanges += cib.changeAtBlockheights().size();

        // at least a single change should occur, because coinbase
        REQUIRE(!cib.changeAtBlockheights().empty());
    }
    auto after = std::chrono::steady_clock::now();
    auto duration = after - before;

    LOG("finished!");

    LOG("\t{:15.3f} ms processing time", std::chrono::duration<double, std::milli>(duration).count());
    LOG("\t{:15.3f} M blocks per second processed ",
        expectedBlockHeight / (1e6 * std::chrono::duration<double>(after - before).count()));

    LOG("\t{:15} bytes processed", mmapedFile.size());
    LOG("\t{:15} changes", totalChanges);
    LOG("\t{:15} blocks", cib.blockHeight() + 1);
    LOG("\t{:15.3f} M changes per second processed ",
        totalChanges / (1e6 * std::chrono::duration<double>(after - before).count()));
    LOG("\t{:15.3f} bytes per change (satoshi & blockheight)", (1.0 * mmapedFile.size() / totalChanges));
    LOG("\t{:15.3f} changes per block on average", (1.0 * totalChanges / (cib.blockHeight() + 1)));
    LOG("\t{:15.3f} bytes per block", (1.0 * mmapedFile.size() / (cib.blockHeight() + 1)));
}

TEST_CASE("decode_skip" * doctest::skip()) {

    static constexpr auto fileName = "../../out/blocks/changes.blk1";
    auto mmapedFile = util::Mmap(fileName);
    if (!mmapedFile.is_open()) {
        throw std::runtime_error(fmt::format("could not open file {}", fileName));
    }

    auto throttler = util::LogThrottler(1000ms);

    const auto* ptr = mmapedFile.begin();
    auto expectedBlockHeight = uint32_t();
    auto blockHeight = uint32_t();

    auto before = std::chrono::steady_clock::now();
    while (ptr != mmapedFile.end()) {
        std::tie(blockHeight, ptr) = buv::ChangesInBlock::skip(ptr);
        LOG_IF(throttler(), "{:4}%: block {}", 100 * mmapedFile.size() / std::distance(mmapedFile.begin(), ptr), blockHeight);
        REQUIRE(blockHeight == expectedBlockHeight);
        ++expectedBlockHeight;
    }
    auto after = std::chrono::steady_clock::now();
    auto duration = after - before;

    LOG("finished!");

    LOG("\t{:15.3f} ms processing time", std::chrono::duration<double, std::milli>(duration).count());
    LOG("\t{:15.3f} M blocks per second processed ",
        expectedBlockHeight / (1e6 * std::chrono::duration<double>(after - before).count()));

    LOG("\t{:15} bytes processed", mmapedFile.size());
    LOG("\t{:15} blocks", expectedBlockHeight);
    LOG("\t{:15.3f} bytes per block", (1.0 * mmapedFile.size() / (expectedBlockHeight + 1)));
}

#include <app/forEachChange.h>
#include <util/LogThrottler.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

using namespace std::literals;

TEST_CASE("decode_change" * doctest::skip()) {
    auto before = std::chrono::steady_clock::now();

    auto throttler = util::LogThrottler(1000ms);

    auto expectedBlockHeight = uint32_t();
    auto totalChanges = size_t();

    buv::forEachChange("../../out/blocks/changes.blk1", [&](buv::ChangesInBlock const& cib) {
        LOG_IF(throttler(), "block {}, {} changes", cib.blockHeight(), cib.changeAtBlockheights().size());
        REQUIRE(cib.blockHeight() == expectedBlockHeight);
        ++expectedBlockHeight;

        totalChanges += cib.changeAtBlockheights().size();

        // at least a single change should occur, because coinbase
        REQUIRE(!cib.changeAtBlockheights().empty());
    });

    auto after = std::chrono::steady_clock::now();
    auto duration = after - before;

    LOG("finished!");
    LOG("\t{:15.3f} ms processing time", std::chrono::duration<double, std::milli>(duration).count());
    LOG("\t{:15.3f} M blocks per second processed ",
        expectedBlockHeight / (1e6 * std::chrono::duration<double>(after - before).count()));

    LOG("\t{:15} changes", totalChanges);
}

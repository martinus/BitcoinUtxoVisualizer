#include <app/forEachChange.h>
#include <util/Throttle.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

using namespace std::literals;

TEST_CASE("decode_change" * doctest::skip()) {
    auto before = std::chrono::steady_clock::now();

    auto throttler = util::ThrottlePeriodic(1000ms);

    auto expectedBlockHeight = uint32_t();
    auto totalChanges = size_t();

    auto file = util::Mmap("/run/media/martinus/big/bitcoin/BitcoinUtxoVisualizer/changes.blk1");
    buv::forEachChange(file, [&](buv::ChangesInBlock const& cib) {
        LOGIF(throttler(), "block {}, {} changes", cib.blockData().blockHeight, cib.changeAtBlockheights().size());
        REQUIRE(cib.blockData().blockHeight == expectedBlockHeight);
        ++expectedBlockHeight;

        totalChanges += cib.changeAtBlockheights().size();

        // at least a single change should occur, because coinbase
        REQUIRE(!cib.changeAtBlockheights().empty());
        return true;
    });

    auto after = std::chrono::steady_clock::now();
    auto duration = after - before;

    LOG("finished!");
    LOG("\t{:15.3f} ms processing time", std::chrono::duration<double, std::milli>(duration).count());
    LOG("\t{:15.3f} M blocks per second processed ",
        expectedBlockHeight / (1e6 * std::chrono::duration<double>(after - before).count()));

    LOG("\t{:15} changes", totalChanges);
}

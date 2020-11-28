#include <app/forEachChange.h>
#include <util/LogThrottler.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

using namespace std::literals;

TEST_CASE("visualizer" * doctest::skip()) {
    auto numChanges = size_t();
    buv::forEachChange("../../out/blocks/changes.blk1", [&](buv::ChangesInBlock const& cib) {
        numChanges += cib.changeAtBlockheights().size();
    });
    fmt::print("{} changes\n", numChanges);
}

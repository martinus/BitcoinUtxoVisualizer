#include <app/forEachChange.h>
#include <buv/Density.h>
#include <util/LogThrottler.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

using namespace std::literals;

TEST_CASE("visualizer" * doctest::skip()) {
    // size_t const width = 3840;
    // size_t const height = 2160;
    // size_t const max_included_density = 444;

    size_t const width = 2560;
    size_t const height = 1440;
    size_t const max_included_density = 1000;

    auto density = buv::Density(width,                   // width
                                height,                  // height
                                1,                       // minimum satoshi
                                10'000ULL * 100'000'000, // max satoshi,
                                0,                       // minimum block height
                                550'000                  // maximum block height
    );

    buv::forEachChange("../../out/blocks/changes.blk1", [&](buv::ChangesInBlock const& cib) {
    });
    fmt::print("{} changes\n", numChanges);
}

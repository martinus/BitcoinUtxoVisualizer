#include <util/args.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>
#include <fmt/format.h>

auto main(int argc, char** argv) -> int {
    util::args::set(argc, argv);

    auto context = doctest::Context();
    context.applyCommandLine(argc, argv);

    // run queries, or run tests unless --no-run is specified
    auto res = context.run();

    // important - query flags (and --exit) rely on the user doing this
    if (context.shouldExit()) {
        // propagate the result of the tests
        return res;
    }
}

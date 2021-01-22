#include <util/BlockHeightProgressBar.h>

#include <doctest.h>

#include <chrono>
#include <thread>

TEST_CASE("progressbar" * doctest::skip()) {
    auto bar = util::BlockHeightProgressBar::create(650000, "download block headers ");

    auto i = size_t();
    while (true) {
        i += 2000U;
        bar->set_progress(i, "{}/{}", i, 650000);
        if (bar->is_completed()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

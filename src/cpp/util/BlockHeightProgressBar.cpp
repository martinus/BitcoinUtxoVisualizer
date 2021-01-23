#include "BlockHeightProgressBar.h"

#include <indicators/indicators.hpp>

#include <fmt/format.h>

using namespace indicators;

namespace util {

class BlockHeightProgressBarImpl : public BlockHeightProgressBar {
    BlockProgressBar mBar;

public:
    // see https://github.com/p-ranav/indicators
    BlockHeightProgressBarImpl(size_t maxProgress, std::string_view what)
        : mBar{option::BarWidth{120},
               option::Start{"["},
               option::End{"]"},
               option::ShowElapsedTime{true},
               option::ShowRemainingTime{true},
               option::MaxProgress{maxProgress},
               option::PrefixText{fmt::format("{:<25}", what)}} {
        show_console_cursor(false);
    }

    void set_progress(size_t progress, std::string&& str) override {
        mBar.set_option(option::PostfixText{std::move(str)});
        mBar.set_progress(progress);
    }

    [[nodiscard]] auto is_completed() const -> bool override {
        return mBar.is_completed();
    }

    void mark_as_completed() override {
        mBar.mark_as_completed();
    }
};

auto BlockHeightProgressBar::create(size_t maxProgress, std::string_view prefixTitle) -> std::unique_ptr<BlockHeightProgressBar> {
    return std::make_unique<BlockHeightProgressBarImpl>(maxProgress, prefixTitle);
}

} // namespace util

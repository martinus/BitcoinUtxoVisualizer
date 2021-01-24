#include "BlockHeightProgressBar.h"

#include <indicators/indicators.hpp>

#include <fmt/format.h>

using namespace indicators;

namespace {

constexpr auto prefixFormat = "{:<15}";

template <typename... Args>
[[nodiscard]] auto defaultBar(size_t maxProgress, std::string_view what, Args&&... args) -> BlockProgressBar {
    return BlockProgressBar{option::BarWidth{120},
                            option::Start{"["},
                            option::End{"]"},
                            option::MaxProgress{maxProgress},
                            option::PrefixText{fmt::format(prefixFormat, what)},
                            std::forward<Args>(args)...};
}

} // namespace

namespace util {

class BlockHeightProgressBarImpl : public BlockHeightProgressBar {
    BlockProgressBar mBar;

public:
    // see https://github.com/p-ranav/indicators
    BlockHeightProgressBarImpl(size_t maxProgress, std::string_view what)
        : mBar{defaultBar(maxProgress, what, option::ShowRemainingTime{true}, option::ShowElapsedTime{true})} {
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

////////

class HeightAndTxProgressBarImpl : public HeightAndTxProgressBar {
    BlockProgressBar mActiveWorkers;
    BlockProgressBar mBarBlocks;
    BlockProgressBar mBarTx;
    MultiProgress<BlockProgressBar, 3> mBars;
    size_t mMaxBlocks{};
    size_t mMaxTx{};

public:
    HeightAndTxProgressBarImpl(size_t maxNumActiveWorkers, size_t maxBlocks, size_t maxTx)
        : mActiveWorkers{defaultBar(maxNumActiveWorkers, "busy threads", option::ShowPercentage{false})}
        , mBarBlocks(defaultBar(maxBlocks, "blocks"))
        , mBarTx(defaultBar(maxTx, "transactions", option::ShowRemainingTime{true}, option::ShowElapsedTime{true}))
        , mBars(mActiveWorkers, mBarBlocks, mBarTx)
        , mMaxBlocks(maxBlocks)
        , mMaxTx(maxTx) {}

    void set_progress(float numActiveWorkers, size_t numBlocks, size_t numTx) override {
        mActiveWorkers.set_option(option::PostfixText{fmt::format("{:.1f} workers", numActiveWorkers)});
        mBarBlocks.set_option(option::PostfixText{fmt::format("{}/{}", numBlocks, mMaxBlocks)});
        mBarTx.set_option(option::PostfixText{fmt::format("{}/{}", numTx, mMaxTx)});

        mBars.set_progress<0>(numActiveWorkers);
        mBars.set_progress<1>(numBlocks);
        mBars.set_progress<2>(numTx);
    }
};

auto HeightAndTxProgressBar::create(size_t maxNumActiveWorkers, size_t maxBlocks, size_t maxTx)
    -> std::unique_ptr<HeightAndTxProgressBar> {
    return std::make_unique<HeightAndTxProgressBarImpl>(maxNumActiveWorkers, maxBlocks, maxTx);
}

} // namespace util

#pragma once

#include <cstddef>
#include <memory>
#include <string_view>

#include <fmt/format.h>

namespace util {

class BlockHeightProgressBar {
public:
    static auto create(size_t maxProgress, std::string_view prefixTitle) -> std::unique_ptr<BlockHeightProgressBar>;

    template <typename... Args>
    void set_progress(size_t progress, char const* format, Args&&... args) {
        set_progress(progress, fmt::format(format, std::forward<Args>(args)...));
    }

    virtual void set_progress(size_t progress, std::string&& str) = 0;
    [[nodiscard]] virtual auto is_completed() const -> bool = 0;
    virtual void mark_as_completed() = 0;

    virtual ~BlockHeightProgressBar() = default;
    BlockHeightProgressBar() = default;

    BlockHeightProgressBar(BlockHeightProgressBar const&) = delete;
    BlockHeightProgressBar(BlockHeightProgressBar&&) = delete;
    auto operator=(BlockHeightProgressBar const&) -> BlockHeightProgressBar& = delete;
    auto operator=(BlockHeightProgressBar&&) -> BlockHeightProgressBar& = delete;
};

} // namespace util

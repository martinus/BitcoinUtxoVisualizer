#pragma once

#include <app/BlockEncoder.h>
#include <util/Mmap.h>

#include <filesystem>

namespace buv {

template <typename Op>
auto forEachChange(util::Mmap const& mmappedFile, Op op) -> buv::ChangesInBlock {
    if (!mmappedFile.is_open()) {
        throw std::runtime_error("file not open");
    }

    auto const* ptr = mmappedFile.begin();
    auto const* end = mmappedFile.end();

    auto cib = buv::ChangesInBlock();
    while (ptr != end) {
        std::tie(cib, ptr) = buv::ChangesInBlock::decode(std::move(cib), ptr);
        // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved)
        if (!op(cib)) {
            return cib;
        }
    }

    return cib;
}

[[nodiscard]] inline auto numBlocks(util::Mmap const& mmappedFile) -> size_t {
    if (!mmappedFile.is_open()) {
        throw std::runtime_error("file not open");
    }

    auto const* ptr = mmappedFile.begin();
    auto const* end = mmappedFile.end();

    auto blockHeight = uint32_t();
    while (ptr != end) {
        std::tie(blockHeight, ptr) = buv::ChangesInBlock::skip(ptr);
    }

    return blockHeight + 1;
}

} // namespace buv

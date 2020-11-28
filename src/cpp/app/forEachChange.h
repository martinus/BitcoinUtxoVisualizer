#pragma once

#include <app/BlockEncoder.h>
#include <util/Mmap.h>

#include <fmt/core.h>

#include <filesystem>

namespace buv {

template <typename Op>
void forEachChange(std::filesystem::path const& blkFilename, Op op) {
    auto mmapedFile = util::Mmap(blkFilename);
    if (!mmapedFile.is_open()) {
        throw std::runtime_error(fmt::format("could not open file {}", blkFilename.string()));
    }

    auto const* ptr = mmapedFile.begin();
    auto const* end = mmapedFile.end();

    auto cib = buv::ChangesInBlock();
    while (ptr != end) {
        std::tie(cib, ptr) = buv::ChangesInBlock::decode(std::move(cib), ptr);
        // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved)
        op(cib);
    }
}

} // namespace buv

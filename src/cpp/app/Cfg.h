#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace buv {
template <typename T>
struct Rect {
    T x{};
    T y{};
    T w{};
    T h{};
};

struct Cfg {
    std::string bitcoinRpcUrl{};

    std::string blkFile{};
    int64_t utxoToChangeNumThreads{};
    int64_t utxoToChangeNumResources{};

    size_t imageWidth{};
    size_t imageHeight{};

    Rect<size_t> graphRect{};
    int64_t minSatoshi{};
    int64_t maxSatoshi{};
    uint32_t startShowAtBlockHeight{};
    uint32_t skipBlocks{1};
    uint32_t repeatLastBlockTimes{0};
    std::string connectionIpAddr = "127.0.0.1";
    uint16_t connectionSocket = 12987;
    std::string colorMap = "viridis";
    size_t colorUpperValueLimit = 4000U;
    std::array<uint8_t, 3> colorHighlightRGB{};
    std::array<uint8_t, 3> colorBackgroundRGB{};
};

auto parseCfg(std::filesystem::path const& cfgFile) -> Cfg;

} // namespace buv

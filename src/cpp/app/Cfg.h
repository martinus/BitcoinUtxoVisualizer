#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace buv {

struct Cfg {
    std::string bitcoinRpcUrl{};
    std::string blockHeadersFile{};

    std::string blkFile{};
    size_t imageWidth{};
    size_t imageHeight{};
    int64_t minSatoshi{};
    int64_t maxSatoshi{};
    uint32_t minBlockHeight{};
    uint32_t maxBlockHeight{};

    uint32_t startShowAtBlockHeight{};
    std::string connectionIpAddr = "127.0.0.1";
    uint16_t connectionSocket = 12987;
    std::string colorMap = "viridis";
    size_t colorUpperValueLimit = 4000U;
    std::array<uint8_t, 3> colorHighlightRGB{};
    std::array<uint8_t, 3> colorBackgroundRGB{};
};

auto parseCfg(std::filesystem::path const& cfgFile) -> Cfg;

} // namespace buv

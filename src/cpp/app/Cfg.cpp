#include "Cfg.h"

#include <util/log.h>

#include <fmt/format.h>
#include <simdjson.h>

namespace {

template <typename T>
[[nodiscard]] auto load(simdjson::dom::element const& data, char const* name) -> T {
    try {
        return data[name].get<T>().value();
    } catch (std::exception const& e) {
        throw std::runtime_error(fmt::format("Could not get '{}': {}", name, e.what()));
    }
}

template <typename T, size_t S>
[[nodiscard]] auto loadArray(simdjson::dom::element const& data, char const* name) -> std::array<T, S> {
    auto ary = std::array<T, S>();
    try {
        auto jsonAry = data[name].get_array();
        if (jsonAry.size() != S) {
            throw std::runtime_error(fmt::format("array size is {} but should be {}", jsonAry.size(), S));
        }

        for (size_t i = 0; i < ary.size(); ++i) {
            if constexpr (std::is_unsigned_v<T>) {
                ary[i] = jsonAry.at(i).get_int64();
            } else {
                ary[i] = jsonAry.at(i).get_uint64();
            }
        }
        return ary;
    } catch (std::exception const& e) {
        throw std::runtime_error(fmt::format("Could not get '{}': {}", name, e.what()));
    }
}

} // namespace

namespace buv {

auto parseCfg(std::filesystem::path const& cfgFile) -> Cfg {
    auto jsonParser = simdjson::dom::parser();
    simdjson::dom::element data = jsonParser.load(cfgFile);

    auto cfg = Cfg();
    LOG("Loading config file {}", cfgFile.string());
    cfg.bitcoinRpcUrl = std::string(load<std::string_view>(data, "bitcoinRpcUrl"));
    cfg.blockHeadersFile = std::string(load<std::string_view>(data, "blockHeadersFile"));
    cfg.blkFile = std::string(load<std::string_view>(data, "blkFile"));
    cfg.imageWidth = load<uint64_t>(data, "imageWidth");
    cfg.imageHeight = load<uint64_t>(data, "imageHeight");

    auto rect = loadArray<size_t, 4>(data, "graphRect");
    cfg.graphRect.x = rect[0];
    cfg.graphRect.y = rect[1];
    cfg.graphRect.w = rect[2];
    cfg.graphRect.h = rect[3];

    cfg.minSatoshi = load<int64_t>(data, "minSatoshi");
    cfg.maxSatoshi = load<int64_t>(data, "maxSatoshi");
    cfg.minBlockHeight = load<uint64_t>(data, "minBlockHeight");
    cfg.maxBlockHeight = load<uint64_t>(data, "maxBlockHeight");
    cfg.startShowAtBlockHeight = load<uint64_t>(data, "startShowAtBlockHeight");
    cfg.skipBlocks = load<uint64_t>(data, "skipBlocks");
    cfg.connectionIpAddr = std::string(load<std::string_view>(data, "connectionIpAddr"));
    cfg.connectionSocket = load<uint64_t>(data, "connectionSocket");
    cfg.colorUpperValueLimit = load<uint64_t>(data, "colorUpperValueLimit");
    cfg.colorMap = std::string(load<std::string_view>(data, "colorMap"));
    cfg.colorHighlightRGB = loadArray<uint8_t, 3>(data, "colorHighlightRGB");
    cfg.colorBackgroundRGB = loadArray<uint8_t, 3>(data, "colorBackgroundRGB");
    return cfg;
}

} // namespace buv

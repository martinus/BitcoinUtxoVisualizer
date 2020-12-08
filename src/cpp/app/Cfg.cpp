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

} // namespace

namespace buv {

auto parseCfg(std::filesystem::path const& cfgFile) -> Cfg {
    auto jsonParser = simdjson::dom::parser();
    simdjson::dom::element data = jsonParser.load(cfgFile);

    auto cfg = Cfg();
    LOG("Loading config file {}", cfgFile.string());
    cfg.blkFile = std::string(load<std::string_view>(data, "blkFile"));
    cfg.pixelWidth = load<uint64_t>(data, "pixelWidth");
    cfg.pixelHeight = load<uint64_t>(data, "pixelHeight");
    cfg.minSatoshi = load<int64_t>(data, "minSatoshi");
    cfg.maxSatoshi = load<int64_t>(data, "maxSatoshi");
    cfg.minBlockHeight = load<uint64_t>(data, "minBlockHeight");
    cfg.maxBlockHeight = load<uint64_t>(data, "maxBlockHeight");
    cfg.startShowAtBlockHeight = load<uint64_t>(data, "startShowAtBlockHeight");
    cfg.connectionIpAddr = std::string(load<std::string_view>(data, "connectionIpAddr"));
    cfg.connectionSocket = load<uint64_t>(data, "connectionSocket");
    cfg.colorUpperValueLimit = load<uint64_t>(data, "colorUpperValueLimit");
    cfg.colorMap = std::string(load<std::string_view>(data, "colorMap"));
    return cfg;
}

} // namespace buv

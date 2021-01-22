#pragma once

#include <util/HttpClient.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fmt/core.h>
#include <vector>

namespace buv {

// see e.g.http://127.0.0.1:8332/rest/headers/2000/000000000000000000086ec6c62d2add3a905f02162a57959e97868c797d1921.json
// more data is available, but 
struct BlockHeader {
    std::array<uint8_t, 32> hash{};
    size_t nTx{};
};

// Fetches a list of all block hashes currently available, in order 0 - x.
auto fetchAllBlockHeaders(char const* bitcoinRpcUrl) -> std::vector<BlockHeader>;

auto fetchAllBlockHeaders(std::unique_ptr<util::HttpClient>& cli) -> std::vector<BlockHeader>;

} // namespace buv

#pragma once

#include <util/HttpClient.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fmt/core.h>
#include <vector>

namespace buv {

using BlockHash = std::array<uint8_t, 32>;

// Fetches a list of all block hashes currently available, in order 0 - x.
auto fetchAllBlockHashes(char const* bitcoinRpcUrl) -> std::vector<BlockHash>;

auto fetchAllBlockHashes(std::unique_ptr<util::HttpClient>& cli) -> std::vector<BlockHash>;

} // namespace buv

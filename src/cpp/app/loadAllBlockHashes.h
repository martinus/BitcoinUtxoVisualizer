#pragma once

#include <util/HttpClient.h>

#include <string>
#include <string_view>
#include <vector>

namespace buv {

// starting from the given block hash, loads all block hashes.
[[nodiscard]] auto loadAllBlockHashes(std::unique_ptr<util::HttpClient> const& cli, std::string_view startBlock)
    -> std::vector<std::string>;

// loads all block hashes, starting from the genesis block.
[[nodiscard]] auto loadAllBlockHashes(std::unique_ptr<util::HttpClient> const& cli) -> std::vector<std::string>;

} // namespace buv

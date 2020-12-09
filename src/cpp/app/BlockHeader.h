#pragma once

#include <util/HttpClient.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace buv {

// Stores all the information received from the blockheaders.
// E.g. see http://127.0.0.1:8332/rest/headers/1/0000000000000000000e5c22c24fd31a29d8ecb2c2cec80d3a9cb75b7aa54f18.json
//
// Also see https://developer.bitcoin.org/reference/block_chain.html
struct BlockHeader {
    std::array<uint8_t, 32> hash{};
    std::array<uint8_t, 32> merkleroot{};
    std::array<uint8_t, 32> chainwork{};
    double difficulty{};
    uint32_t confirmations{};
    uint32_t height{};
    uint32_t version{};
    std::chrono::system_clock::time_point time{};
    std::chrono::system_clock::time_point mediantime{};
    uint32_t nonce{};
    std::array<uint8_t, 4> bits{};
    uint32_t nTx{};

    // Loads everything from bitcoin core node
    static auto fetch(char const* bitcoinRpcUrl) -> std::vector<BlockHeader>;
    static auto fetch(std::unique_ptr<util::HttpClient>& cli) -> std::vector<BlockHeader>;

    // Writes everything
    static void write(std::vector<BlockHeader> const& blockHeaders, std::filesystem::path const& filename);

    // loads all from serialized file
    static auto load(std::filesystem::path const& filename) -> std::vector<BlockHeader>;
};

} // namespace buv

#include <doctest.h>
#include <fmt/format.h>

#include <filesystem>
#include <string_view>

TEST_CASE("utxo_to_change") {
    static constexpr auto bitcoinRpcUrl = std::string_view("http://127.0.0.1:8332");
    static constexpr auto dataDir = "../../out/blocks";

    std::filesystem::create_directories(dataDir);

    fmt::print("created {}, accessing bitcoin at {}\n", dataDir, bitcoinRpcUrl);
}

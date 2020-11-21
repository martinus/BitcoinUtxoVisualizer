#include <app/BlockEncoder.h>
#include <app/Utxo.h>
#include <util/HttpClient.h>
#include <util/hex.h>
#include <util/log.h>

#include <doctest.h>
#include <fmt/format.h>
#include <simdjson.h>

#include <filesystem>
#include <fstream>
#include <string_view>

using namespace std::literals;

class Throttler {
    std::chrono::nanoseconds mDelay{};
    std::chrono::steady_clock::time_point mNextDeadline{};

public:
    explicit Throttler(std::chrono::nanoseconds delay)
        : mDelay(delay) {}

    // true if enough time has passed
    [[nodiscard]] auto operator()() -> bool {
        auto now = std::chrono::steady_clock::now();
        if (now >= mNextDeadline) {
            mNextDeadline = now + mDelay;
            return true;
        }
        return false;
    }
};

void integrateBlockData(simdjson::dom::element const& blockData, buv::Utxo& utxo) {
    auto txIdx = size_t();
    for (auto const& tx : blockData["tx"]) {
        // remove all inputs consumed by this transaction from utxo
        if (txIdx != 0) {
            // first transaction is coinbase, has no inputs
            auto vinIdx = size_t();
            for (auto const& vin : tx["vin"]) {
                auto sourceTxid = util::fromHex<16>(vin["txid"].get_c_str());
                auto sourceVout = static_cast<uint16_t>(vin["vout"].get_uint64());

                auto unspentSourceOutputs = utxo.find(sourceTxid);
                if (unspentSourceOutputs == utxo.end()) {
                    throw std::runtime_error("DAMN! did not find txid");
                }
                auto voutAmount = unspentSourceOutputs->second.find(sourceVout);
                if (voutAmount == unspentSourceOutputs->second.end()) {
                    throw std::runtime_error("DAMN! output not there");
                }

                // found an output that's spent
                auto satoshi = voutAmount->second;
                (void)satoshi;

                ++vinIdx;
            }
        }
        ++txIdx;
    }
}

TEST_CASE("utxo_to_change" * doctest::skip()) {
    static constexpr auto bitcoinRpcUrl = "http://127.0.0.1:8332";
    static constexpr auto dataDir = "../../out/blocks";
    static constexpr auto genesisBlock = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";

    std::filesystem::create_directories(dataDir);

    fmt::print("created {}, accessing bitcoin at {}\n", dataDir, bitcoinRpcUrl);

    auto cli = util::HttpClient::create(bitcoinRpcUrl);
    auto jsonParser = simdjson::dom::parser();

    auto nextblockhash = std::string_view(genesisBlock);

    auto throttler = Throttler(1s);

    auto fout = std::ofstream(std::filesystem::path(dataDir) / "changes.blk1", std::ios::binary | std::ios::out);
    auto utxo = buv::Utxo();

    while (true) {
        auto json = cli->get("/rest/block/{}.json", nextblockhash);
        simdjson::dom::element blockData = jsonParser.parse(json);

        auto cib = buv::ChangesInBlock();
        cib.beginBlock(blockData["height"].get_uint64());
        integrateBlockData(blockData, utxo);
        LOG_IF(throttler() ? util::Log::show : util::Log::hide, "height={}, bytes={}", cib.blockHeight(), json.size());

        if (blockData["nextblockhash"].get(nextblockhash) != 0U) {
            // last block, stop everything
            break;
        }
    }
}

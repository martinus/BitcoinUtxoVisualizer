#include <app/BlockEncoder.h>
#include <app/UtxoV2.h>
#include <app/loadAllBlockHashes.h>
#include <util/HttpClient.h>
#include <util/LogThrottler.h>
#include <util/hex.h>
#include <util/log.h>
#include <util/parallelToSequential.h>
#include <util/reserve.h>

#include <doctest.h>
#include <fmt/format.h>
#include <simdjson.h>

#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>

using namespace std::literals;

namespace {

[[nodiscard]] auto integrateBlockData(simdjson::dom::element const& blockData, buv::Utxo& utxo) -> buv::ChangesInBlock {
    auto cib = buv::ChangesInBlock();
    cib.beginBlock(blockData["height"].get_uint64());

    auto isCoinbaseTx = true;
    for (auto const& tx : blockData["tx"]) {
        // remove all inputs consumed by this transaction from utxo
        if (!isCoinbaseTx) {
            // first transaction is coinbase, has no inputs
            for (auto const& vin : tx["vin"]) {
                // txid & voutNr exactly define what is spent
                auto sourceTxid = util::fromHex<buv::txidPrefixSize>(vin["txid"].get_c_str());
                auto sourceVout = static_cast<uint16_t>(vin["vout"].get_uint64());

                auto [satoshi, blockHeight] = utxo.remove(sourceTxid, sourceVout);

                // found an output that's spent! negative amount, because it's spent
                cib.addChange(-satoshi, blockHeight);
            }
        } else {
            isCoinbaseTx = false;
        }

        // add all outputs from this transaction to the utxo
        auto txid = util::fromHex<buv::txidPrefixSize>(tx["txid"].get_c_str());
        auto inserter = utxo.inserter(txid, cib.blockHeight());

        auto n = 0;
        for (auto const& vout : tx["vout"]) {
            auto sat = std::llround(vout["value"].get_double() * 100'000'000);
            inserter.insert(n, sat);
            cib.addChange(sat, cib.blockHeight());
            ++n;
        }
    }
    cib.finalizeBlock();
    return cib;
}

struct ResourceData {
    std::unique_ptr<util::HttpClient> cli{};
    std::string jsonData{};
    simdjson::dom::parser jsonParser{};
    simdjson::dom::element blockData{};
};

} // namespace

TEST_CASE("utxo_to_change" * doctest::skip()) {
    static constexpr auto bitcoinRpcUrl = "http://127.0.0.1:8332";
    static constexpr auto dataDir = "../../out/blocks";

    // genesis block, height 0
    static constexpr auto startBlock = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";

    // block 645693
    // static constexpr auto startBlock = "0000000000000000000e5c22c24fd31a29d8ecb2c2cec80d3a9cb75b7aa54f18";

    std::filesystem::create_directories(dataDir);

    fmt::print("created {}, accessing bitcoin at {}\n", dataDir, bitcoinRpcUrl);
    auto cli = util::HttpClient::create(bitcoinRpcUrl);
    auto jsonParser = simdjson::dom::parser();

    auto allBlockHashes = buv::loadAllBlockHashes(cli, startBlock);

    auto throttler = util::LogThrottler(1s);
    // auto utxoDumpThrottler = util::LogThrottler(20s);

    auto fout = std::ofstream(std::filesystem::path(dataDir) / "changes.blk1", std::ios::binary | std::ios::out);
    auto utxo = std::make_unique<buv::Utxo>();

    auto resources = std::vector<ResourceData>(std::thread::hardware_concurrency() * 2);
    for (auto& resource : resources) {
        resource.cli = util::HttpClient::create(bitcoinRpcUrl);
    }

    util::parallelToSequential(
        util::SequenceId{allBlockHashes.size()},
        util::ResourceId{resources.size()},
        util::ConcurrentWorkers{std::thread::hardware_concurrency()},

        [&](util::ResourceId resourceId, util::SequenceId sequenceId) {
            auto& res = resources[resourceId.count()];
            auto& hash = allBlockHashes[sequenceId.count()];

            res.jsonData = res.cli->get("/rest/block/{}.json", hash);
            res.blockData = res.jsonParser.parse(res.jsonData);
        },
        [&](util::ResourceId resourceId, util::SequenceId /*sequenceId*/) {
            auto& res = resources[resourceId.count()];
            auto cib = integrateBlockData(res.blockData, *utxo);

            LOGIF(throttler(), "height={}, bytes={}. utxo: {}", cib.blockHeight(), res.jsonData.size(), *utxo);

            // free the memory of the resource. Also helps find bugs (operating on old data. Not that it has ever happened, but
            // still)
            res.jsonData = std::string();

            fout << cib.encode();

#if 0
            // if enough time has passed, dump utxo, free memory, then load it again.
            if (utxoDumpThrottler() == util::Log::show) {
                auto utxoDumpFilename = std::filesystem::path(dataDir) / "utxo.dump";
                LOG("storing UTXO...");
                buv::storeUtxo(*utxo, utxoDumpFilename);
                // free the memory before loading
                LOG("freeing UTXO memory...");
                utxo = nullptr;
                LOG("waiting 5 seconds...");
                std::this_thread::sleep_for(5s);
                LOG("loading UTXO...");
                utxo = std::make_unique<buv::Utxo>();
                utxo = buv::loadUtxo(utxoDumpFilename);
                LOG("loading UTXO done!");
            }
#endif
        });
}

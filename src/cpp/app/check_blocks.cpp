#include <app/BlockHeader.h>
#include <util/HttpClient.h>
#include <util/Throttle.h>
#include <util/hex.h>
#include <util/log.h>
#include <util/parallelToSequential.h>

#include <doctest.h>
#include <simdjson.h>

// 3637.720 src/cpp/app/utxo_to_change.cpp(174) | height=557166, bytes=7128215. utxo: 34470525 entries

namespace {

struct ResourceData {
    std::unique_ptr<util::HttpClient> cli{};
    std::string jsonData{};
    simdjson::dom::parser jsonParser{};
    simdjson::dom::element blockData{};
};

} // namespace

using namespace std::literals;

TEST_CASE("check_blocks" * doctest::skip()) {
    static constexpr auto bitcoinRpcUrl = "http://127.0.0.1:8332";
    auto cli = util::HttpClient::create(bitcoinRpcUrl);

    auto allBlockHeaders = buv::BlockHeader::fetch(cli);

    LOG("got {} blocks", allBlockHeaders.size());

    auto resources = std::vector<ResourceData>(std::thread::hardware_concurrency() * 2);
    for (auto& resource : resources) {
        resource.cli = util::HttpClient::create(bitcoinRpcUrl);
    }

    auto throttler = util::ThrottlePeriodic(1s);

    util::parallelToSequential(
        util::SequenceId{allBlockHeaders.size()},
        util::ResourceId{resources.size()},
        util::ConcurrentWorkers{std::thread::hardware_concurrency()},
        [&](util::ResourceId resourceId, util::SequenceId sequenceId) {
            auto& res = resources[resourceId.count()];
            auto hash = util::toHex(allBlockHeaders[sequenceId.count()].hash);

            res.jsonData = res.cli->get("/rest/block/{}.json", hash);
            res.blockData = res.jsonParser.parse(res.jsonData);
        },
        [&](util::ResourceId resourceId, util::SequenceId sequenceId) {
            auto& res = resources[resourceId.count()];
            auto hash = util::toHex(allBlockHeaders[sequenceId.count()].hash);

            LOGIF(throttler(), "block {} at {}, resource {}", hash, sequenceId.count(), resourceId.count());

#if 0

            auto isCoinbaseTx = true;
            for (auto const& tx : res.blockData["tx"]) {
                if (!isCoinbaseTx) {
                    // first transaction is coinbase, has no inputs
                    for (auto const& vin : tx["vin"]) {
                        auto sourceTxid = util::fromHex<16>(vin["txid"].get_c_str());
                        auto sourceVout = static_cast<uint16_t>(vin["vout"].get_uint64());

                        (void)sourceTxid;
                        (void)sourceVout;
                    }
                } else {
                    isCoinbaseTx = false;
                }

                // add all outputs from this transaction to the utxo
                auto txid = util::fromHex<16>(tx["txid"].get_c_str());
                (void)txid;
                for (auto const& vout : tx["vout"]) {
                    auto sat = std::llround(vout["value"].get_double() * 100'000'000);
                    (void)sat;
                }
            }
#endif
            // free the data
            res.jsonData = std::string();
            // res.jsonParser = simdjson::dom::parser();
        });
}

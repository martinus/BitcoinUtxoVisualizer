#include <app/Utxo.h>

#include <doctest.h>
#include <util/log.h>

#include <unordered_map>

static_assert(sizeof(buv::VOutNr) == 2);
static_assert(sizeof(buv::Satoshi) == sizeof(uint64_t));
static_assert(sizeof(robin_hood::pair<buv::VOutNr, buv::Satoshi>) == sizeof(buv::VOutNr) + sizeof(buv::Satoshi), "no padding");

// creates the data structure
TEST_CASE("utxo") {
    auto utxo = buv::Utxo();
    LOG("{} B for buv::Utxo::value_type", sizeof(buv::Utxo::value_type));
    LOG("{} B for buv::Utxo::key_type", sizeof(buv::Utxo::key_type));
    LOG("{} B for buv::Utxo::mapped_type", sizeof(buv::Utxo::mapped_type));

    LOG("");

    using InnerMap = buv::UtxoPerTx;
    LOG("{} B for InnerMap::value_type", sizeof(InnerMap::value_type));
    LOG("{} B for InnerMap::key_type", sizeof(InnerMap::key_type));
    LOG("{} B for InnerMap::mapped_type", sizeof(InnerMap::mapped_type));

    LOG("");

    LOG("{} B robin_hood::unordered_flat_map<buv::VOutNr, buv::Satoshi>",
        sizeof(robin_hood::unordered_flat_map<buv::VOutNr, buv::Satoshi>));
    LOG("{} B robin_hood::unordered_node_map<buv::VOutNr, buv::Satoshi>",
        sizeof(robin_hood::unordered_node_map<buv::VOutNr, buv::Satoshi>));
    LOG("{} B std::unordered_map<buv::VOutNr, buv::Satoshi>", sizeof(std::unordered_map<buv::VOutNr, buv::Satoshi>));
}

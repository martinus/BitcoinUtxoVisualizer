#include <app/Utxo.h>

#include <doctest.h>
#include <fmt/format.h>

#include <unordered_map>

static_assert(sizeof(buv::VOutNr) == 2);
static_assert(sizeof(buv::Satoshi) == sizeof(uint64_t));
static_assert(sizeof(robin_hood::pair<buv::VOutNr, buv::Satoshi>) == sizeof(buv::VOutNr) + sizeof(buv::Satoshi), "no padding");

// creates the data structure
TEST_CASE("utxo") {
    auto utxo = buv::Utxo();
    fmt::print("{} B robin_hood::unordered_flat_map<buv::VOutNr, buv::Satoshi>\n",
               sizeof(robin_hood::unordered_flat_map<buv::VOutNr, buv::Satoshi>));
    fmt::print("{} B robin_hood::unordered_node_map<buv::VOutNr, buv::Satoshi>\n",
               sizeof(robin_hood::unordered_node_map<buv::VOutNr, buv::Satoshi>));
    fmt::print("{} B std::unordered_map<buv::VOutNr, buv::Satoshi>\n", sizeof(std::unordered_map<buv::VOutNr, buv::Satoshi>));
}

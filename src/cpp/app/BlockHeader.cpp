#include "BlockHeader.h"

#include <util/HttpClient.h>
#include <util/hex.h>
#include <util/writeBinary.h>

#include <simdjson.h>

#include <chrono>
#include <fstream>

namespace buv {

auto BlockHeader::fetch(std::unique_ptr<util::HttpClient>& cli) -> std::vector<BlockHeader> {
    auto jsonParser = simdjson::dom::parser();

    // genesis block
    auto block = std::string_view("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");

    auto blockHeaders = std::vector<BlockHeader>();
    while (true) {
        auto json = cli->get("/rest/headers/2000/{}.json", block);
        simdjson::dom::array data = jsonParser.parse(json);

        // auto nextblockhash = std::string_view();
        for (simdjson::dom::element e : data) {
            auto bh = BlockHeader();
            bh.hash = util::fromHex<32>(e["hash"].get_string().value().data());
            bh.merkleroot = util::fromHex<32>(e["merkleroot"].get_string().value().data());
            bh.chainwork = util::fromHex<32>(e["chainwork"].get_string().value().data());
            bh.difficulty = e["difficulty"];
            bh.confirmations = e["confirmations"].get_uint64();
            bh.height = e["height"].get_uint64();
            bh.version = e["version"].get_uint64();
            bh.time = std::chrono::system_clock::time_point(std::chrono::seconds(e["time"].get_int64().value()));
            bh.mediantime = std::chrono::system_clock::time_point(std::chrono::seconds(e["mediantime"].get_int64().value()));
            bh.nonce = e["nonce"].get_uint64();
            bh.bits = util::fromHex<4>(e["bits"].get_string().value().data());
            bh.nTx = e["nTx"].get_uint64();
            blockHeaders.push_back(bh);
        }

        simdjson::dom::element last = data.at(data.size() - 1);
        if (last["nextblockhash"].get(block) != 0U) {
            // field not found, break
            break;
        }

        // it's ok to use std::string_view for block, because it is available until the parse() call, at which point we already
        // have used it.
    }

    return blockHeaders;
}

auto BlockHeader::fetch(char const* bitcoinRpcUrl) -> std::vector<BlockHeader> {
    auto cli = util::HttpClient::create(bitcoinRpcUrl);
    return fetch(cli);
}

// Writes everything
void BlockHeader::write(std::vector<BlockHeader> const& blockHeaders, std::filesystem::path const& filename) {
    auto fout = std::ofstream(filename, std::ios::binary);
    util::writeBinary<uint64_t>(fout, blockHeaders.size());
    for (auto const& bh : blockHeaders) {
        util::writeBinary(fout, bh);
    }
}

// loads all from serialized file
auto BlockHeader::load(std::filesystem::path const& filename) -> std::vector<BlockHeader> {
    auto fin = std::ifstream(filename, std::ios::binary);
    auto numElements = util::readBinary<uint64_t>(fin);

    auto blockHeaders = std::vector<BlockHeader>();
    blockHeaders.resize(numElements);
    for (uint64_t i = 0; i < numElements; ++i) {
        util::readBinary<BlockHeader>(fin, blockHeaders[i]);
    }
    return blockHeaders;
}

} // namespace buv

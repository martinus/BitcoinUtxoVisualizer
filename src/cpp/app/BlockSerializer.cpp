#include "BlockSerializer.h"

#include <util/BinaryStreamWriter.h>
#include <util/VarInt.h>

#include <fmt/format.h>

namespace buv {

void BlockSerializer::beginBlock(uint32_t blockHeight) {
    mBlockHeight = blockHeight;
    mSatoshiAndBlockheight.clear();
}

void BlockSerializer::endBlock() {
    std::sort(mSatoshiAndBlockheight.begin(), mSatoshiAndBlockheight.end());
}

void BlockSerializer::addSpentOutput(uint32_t blockHeight, uint64_t amountSatoshi) {
    mSatoshiAndBlockheight.emplace_back(amountSatoshi, blockHeight);
}

void BlockSerializer::serialize(std::string& data) const {
    data.clear();

    data += std::string_view("BLK0");
    data.append(reinterpret_cast<char const*>(&mBlockHeight), sizeof(mBlockHeight));

    // skip 4 bytes, which will later contain the size of the remaining payload. This can be used to quickly skip to the next
    // block.
    data.append(4, '\0');

    // now comes the data in mSatoshiAndBlockheight. Sorted by satoshi, so the satoshi's only go up.
    // We only store the difference to the previous satoshi amount, encoded as unsigned varint. Thus,
    // the amounts will be stored quite compact. Only thing better would be golomb coded sets
    // https://en.wikipedia.org/wiki/Golomb_coding
    //
    // We also store diffs of block size, but they will be encoded as signed integers, because they can be quite random.
    // already sorted in finishBlock()

    auto prevSatoshi = uint64_t();
    auto prevBlockheight = int64_t();
    auto varInt = util::VarInt();
    for (auto const& [satoshi, blockheight] : mSatoshiAndBlockheight) {
        data += varInt.encode<uint64_t>(satoshi - prevSatoshi);
        data += varInt.encode<int64_t>(static_cast<int64_t>(blockheight) - prevBlockheight);

        prevSatoshi = satoshi;
        prevBlockheight = blockheight;
    }

    // finally, fill in the payload size
    // "BLK0" + blockheight + payloadSize
    auto payloadSize = static_cast<uint32_t>(data.size() - (4U + 4U + 4U));
    std::memcpy(data.data() + (4U + 4U), &payloadSize, 4U);
}

} // namespace buv

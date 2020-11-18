#include "BlockSerializer.h"

#include <util/BinaryStreamWriter.h>
#include <util/VarInt.h>

#include <fmt/format.h>

namespace buv {

BlockSerializer::BlockSerializer(std::filesystem::path const& filename)
    : mOut(filename, std::ios::binary) {
    if (!mOut.is_open()) {
        throw std::runtime_error("could not open file");
    }
}

void BlockSerializer::beginBlock(uint32_t blockHeight) {
    mCurrentBlockHeight = blockHeight;
}

void BlockSerializer::add(uint32_t blockHeight, uint64_t amountSatoshi) {
    mSatoshiAndBlockheight.emplace_back(amountSatoshi, blockHeight);
}

void BlockSerializer::finishBlock() {
    std::sort(mSatoshiAndBlockheight.begin(), mSatoshiAndBlockheight.end());

    // first serialize everything into mTmp, so we know exactly how many bytes the block has
    auto prevSatoshi = uint64_t();
    auto prevBlockheight = int64_t();
    auto varInt = VarInt();
    for (auto const& [satoshi, blockheight] : mSatoshiAndBlockheight) {
        mTmp += varInt.encode(satoshi - prevSatoshi); // guaranteed to not have an overflow
        mTmp += varInt.encode(static_cast<int64_t>(blockheight) - prevBlockheight);

        prevSatoshi = satoshi;
        prevBlockheight = blockheight;
    }

    // now dump everything
    auto bsw = util::BinaryStreamWriter(&mOut);
    bsw.write<4>("BLK\0");
    bsw.write<4>(mCurrentBlockHeight);
    bsw.write<4>(static_cast<uint32_t>(mTmp.size()));
    mOut.write(mTmp.data(), mTmp.size());

    mCurrentBlockHeight = 0;
    mSatoshiAndBlockheight.clear();
    mTmp.clear();
}

} // namespace buv

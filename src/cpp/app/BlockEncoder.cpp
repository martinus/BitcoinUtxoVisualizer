#include "BlockEncoder.h"

#include <util/BinaryStreamWriter.h>
#include <util/VarInt.h>
#include <util/hex.h>
#include <util/writeBinary.h>

#include <fmt/format.h>

namespace buv {

auto ChangesInBlock::beginBlock(uint32_t blockHeight) -> BlockData& {
    mIsFinalized = false;
    mBlockData = {};
    mBlockData.blockHeight = blockHeight;
    mChangeAtBlockheights.clear();
    return mBlockData;
}

void ChangesInBlock::finalizeBlock() {
    if (mIsFinalized) {
        throw std::runtime_error("finalizeBlock() has already been called, not needed any more");
    }
    std::sort(mChangeAtBlockheights.begin(), mChangeAtBlockheights.end());
    mIsFinalized = true;
}

void ChangesInBlock::addChange(int64_t satoshi, uint32_t blockHeight) {
    if (satoshi > 0 && blockHeight != mBlockData.blockHeight) {
        throw std::runtime_error("satoshi > 0 but blockheight does not match!");
    }
    if (mIsFinalized) {
        throw std::runtime_error("can't add an amount after finalizeBlock()");
    }
    mChangeAtBlockheights.emplace_back(satoshi, blockHeight);
}

auto ChangesInBlock::encode() const -> std::string {
    if (!mIsFinalized) {
        throw std::runtime_error("can't encode finalizedBlock() was not called");
    }

    auto data = std::string();

    data += std::string_view("BLK\x02");
    data.append(reinterpret_cast<char const*>(&mBlockData.blockHeight), sizeof(mBlockData.blockHeight));

    // skip 4 bytes, which will later contain the size of the remaining payload. This can be used to quickly skip to the next
    // block.
    data.append(4, '\0');

    // header info
    auto varIntEncoder = util::VarInt();
    util::writeArray<32>(mBlockData.hash, data);
    util::writeArray<32>(mBlockData.merkleRoot, data);
    util::writeArray<32>(mBlockData.chainWork, data);
    util::writeArray<8>(mBlockData.difficultyArray, data);
    util::writeBinary<4>(mBlockData.version, data);
    util::writeBinary<4>(mBlockData.time, data);
    util::writeBinary<4>(mBlockData.medianTime, data);
    util::writeBinary<4>(mBlockData.nonce, data);
    util::writeArray<4>(mBlockData.bits, data);
    data += varIntEncoder.encode<uint32_t>(mBlockData.nTx);
    data += varIntEncoder.encode<uint32_t>(mBlockData.size);
    data += varIntEncoder.encode<uint32_t>(mBlockData.strippedSize);
    data += varIntEncoder.encode<uint32_t>(mBlockData.weight);

    if (!mChangeAtBlockheights.empty()) {
        // now comes the data in mChangeAtBlockheight. Sorted by satoshi, so the satoshi's only increase.
        // The first entry will probably be negative, if any old amount was spent. That is encoded as var_int.
        //
        // We only store the difference to the previous satoshi amount for the following entries, encoded as unsigned varint.
        // Thus, the amounts will be stored quite compact. Only thing better would be golomb coded sets
        // https://en.wikipedia.org/wiki/Golomb_coding
        //
        // We also store diffs of block size, but they will be encoded as signed integers, because they can be quite random.
        // already sorted in finishBlock()

        auto it = mChangeAtBlockheights.begin();
        data += varIntEncoder.encode<int64_t>(it->satoshi());
        data += varIntEncoder.encode<uint64_t>(it->blockHeight());

        auto pre = it;
        ++it;

        while (it != mChangeAtBlockheights.end()) {
            // the amount diff will always be positive since its sorted, so we can serialize an uint
            data += varIntEncoder.encode<uint64_t>(it->satoshi() - pre->satoshi());

            // only encode blockheight if satoshi is negative. Any satoshi that is positive will have the current block.
            if (it->satoshi() <= 0) {
                // diff of blockheight can be negative as well
                data += varIntEncoder.encode<int64_t>(static_cast<int64_t>(it->blockHeight()) -
                                                      static_cast<int64_t>(pre->blockHeight()));
            }
            pre = it;
            ++it;
        }

        // finally, fill in the payload size
        // "BLKx" + blockheight + payloadSize
        auto payloadSize = static_cast<uint32_t>(data.size() - (4U + 4U + 4U));
        std::memcpy(data.data() + (4U + 4U), &payloadSize, 4U);
    }

    return data;
}

[[nodiscard]] auto ChangesInBlock::blockData() const noexcept -> BlockData const& {
    return mBlockData;
}

[[nodiscard]] auto ChangesInBlock::changeAtBlockheights() const noexcept -> std::vector<ChangeAtBlockheight> const& {
    return mChangeAtBlockheights;
}

namespace {

// parse the header and returns
struct Header {
    uint32_t magicMarker{};
    uint32_t blockHeight{};
    uint32_t numBytes{};
};
static_assert(sizeof(Header) == 4U + 4U + 4U);

// parses header, returns the header and pointer to payload
auto parseHeader(char const* ptr) -> std::pair<Header, char const*> {
    auto header = Header();
    std::memcpy(&header, ptr, sizeof(Header));

    if (header.magicMarker != uint32_t(0x024b4c42)) {
        throw std::runtime_error("Decoding error, 'BLK\\2' does not match");
    }

    return std::make_pair(header, ptr + sizeof(Header));
}

} // namespace

auto ChangesInBlock::skip(char const* ptr) -> std::pair<uint32_t, char const*> {
    auto [header, payloadPtr] = parseHeader(ptr);
    return std::make_pair(header.blockHeight, payloadPtr + header.numBytes);
}

[[nodiscard]] auto ChangesInBlock::operator==(ChangesInBlock const& other) const noexcept -> bool {
    return mBlockData.blockHeight == other.mBlockData.blockHeight && mChangeAtBlockheights == other.mChangeAtBlockheights &&
           mIsFinalized == other.mIsFinalized;
}

[[nodiscard]] auto ChangesInBlock::operator!=(ChangesInBlock const& other) const noexcept -> bool {
    return !(*this == other);
}

auto ChangesInBlock::decode(char const* ptr) -> std::pair<ChangesInBlock, char const*> {
    return decode(ChangesInBlock(), ptr);
}

auto ChangesInBlock::decode(ChangesInBlock&& reusableChanges, char const* ptr) -> std::pair<ChangesInBlock, char const*> {
    auto [header, payloadPtr] = parseHeader(ptr);
    reusableChanges.mChangeAtBlockheights.clear();

    auto& bd = reusableChanges.mBlockData;
    bd.blockHeight = header.blockHeight;

    const auto* endPtr = payloadPtr + header.numBytes;

    // decode block header info
    util::read<32>(payloadPtr, bd.hash);
    util::read<32>(payloadPtr, bd.merkleRoot);
    util::read<32>(payloadPtr, bd.chainWork);
    util::read<8>(payloadPtr, bd.difficultyArray);
    util::read<4>(payloadPtr, bd.version);
    util::read<4>(payloadPtr, bd.time);
    util::read<4>(payloadPtr, bd.medianTime);
    util::read<4>(payloadPtr, bd.nonce);
    util::read<4>(payloadPtr, bd.bits);
    util::VarInt::decode<uint32_t>(bd.nTx, payloadPtr);
    util::VarInt::decode<uint32_t>(bd.size, payloadPtr);
    util::VarInt::decode<uint32_t>(bd.strippedSize, payloadPtr);
    util::VarInt::decode<uint32_t>(bd.weight, payloadPtr);

    // decode transaction info

    auto satoshi = int64_t();
    util::VarInt::decode<int64_t>(satoshi, payloadPtr);

    // use a tmp variable so we con decode as uint
    auto tmpBlockHeight = uint64_t();
    util::VarInt::decode<uint64_t>(tmpBlockHeight, payloadPtr);
    reusableChanges.mChangeAtBlockheights.emplace_back(satoshi, tmpBlockHeight);

    auto blockHeight = int64_t(tmpBlockHeight);

    while (payloadPtr < endPtr) {
        auto diffSatoshi = uint64_t();
        auto diffBlockheight = int64_t();

        util::VarInt::decode<uint64_t>(diffSatoshi, payloadPtr);
        satoshi += diffSatoshi;

        if (satoshi <= 0) {
            // only decode blockheight if satoshi was spent
            util::VarInt::decode<int64_t>(diffBlockheight, payloadPtr);
            blockHeight += diffBlockheight;
            reusableChanges.mChangeAtBlockheights.emplace_back(satoshi, blockHeight);
        } else {
            reusableChanges.mChangeAtBlockheights.emplace_back(satoshi, header.blockHeight);
        }
    }

    return std::make_pair(std::move(reusableChanges), payloadPtr);
}

} // namespace buv

namespace fmt {

auto formatter<buv::BlockData>::parse(format_parse_context& ctx) -> format_parse_context::iterator {
    const auto* it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
        throw format_error("invalid format for buv::BlockData");
    }
    return it;
}

auto formatter<buv::BlockData>::format(buv::BlockData const& bd, format_context& ctx) -> format_context::iterator {
    return format_to(
        ctx.out(),
        "\n{:>70} hash\n{:70} strippedsize\n{:70} size\n{:70} weight\n{:70} height\n{:#70x} version\n{:>70} merkleroot\n{:70} time\n{:70} medianTime\n{:70} nonce\n{:>70} bits\n{:70} difficulty\n{:>70} chainWork\n{:70} nTx",
        util::toHex(bd.hash),
        bd.strippedSize,
        bd.size,
        bd.weight,
        bd.blockHeight,
        bd.version,
        util::toHex(bd.merkleRoot),
        bd.time,
        bd.medianTime,
        bd.nonce,
        util::toHex(bd.bits),
        bd.difficulty(),
        util::toHex(bd.chainWork),
        bd.nTx);
}

} // namespace fmt

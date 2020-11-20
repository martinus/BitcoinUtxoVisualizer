#include "BlockEncoder.h"

#include <util/BinaryStreamWriter.h>
#include <util/VarInt.h>

#include <fmt/format.h>

namespace buv {

SatoshiAndBlockheight::SatoshiAndBlockheight(uint64_t satoshi, uint32_t blockHeight)
    : mSatoshi(satoshi)
    , mBlockHeight(blockHeight) {}

[[nodiscard]] auto SatoshiAndBlockheight::satoshi() const noexcept -> uint64_t {
    return mSatoshi;
}

[[nodiscard]] auto SatoshiAndBlockheight::blockHeight() const noexcept -> uint32_t {
    return mBlockHeight;
}

[[nodiscard]] auto SatoshiAndBlockheight::operator<(SatoshiAndBlockheight const& other) const noexcept -> bool {
    if (mSatoshi != other.mSatoshi) {
        return mSatoshi < other.mSatoshi;
    }
    return mBlockHeight < other.mBlockHeight;
}

[[nodiscard]] auto SatoshiAndBlockheight::operator==(SatoshiAndBlockheight const& other) const noexcept -> bool {
    return mSatoshi == other.mSatoshi && mBlockHeight == other.mBlockHeight;
}

[[nodiscard]] auto SatoshiAndBlockheight::operator!=(SatoshiAndBlockheight const& other) const noexcept -> bool {
    return !(*this == other);
}

////

void ChangesInBlock::beginBlock(uint32_t blockHeight) {
    mIsFinalized = false;
    mBlockHeight = blockHeight;
    mSatoshiAndBlockheights.clear();
}

void ChangesInBlock::finalizeBlock() {
    if (mIsFinalized) {
        throw std::runtime_error("finalizeBlock() has already been called, not needed any more");
    }
    std::sort(mSatoshiAndBlockheights.begin(), mSatoshiAndBlockheights.end());
    mIsFinalized = true;
}

void ChangesInBlock::addChange(uint64_t satoshi, uint32_t blockHeight) {
    if (mIsFinalized) {
        throw std::runtime_error("can't add an amount after finalizeBlock()");
    }
    mSatoshiAndBlockheights.emplace_back(satoshi, blockHeight);
}

auto ChangesInBlock::encode() const -> std::string {
    if (!mIsFinalized) {
        throw std::runtime_error("can't encode finalizedBlock() was not called");
    }

    auto data = std::string();

    data += std::string_view("BLK\x01");
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
    auto varIntEncoder = util::VarInt();

    auto prev = SatoshiAndBlockheight(0, 0);
    for (auto const& now : mSatoshiAndBlockheights) {
        data += varIntEncoder.encode<uint64_t>(now.satoshi() - prev.satoshi());
        data += varIntEncoder.encode<int64_t>(static_cast<int64_t>(now.blockHeight()) - static_cast<int64_t>(prev.blockHeight()));

        prev = now;
    }

    // finally, fill in the payload size
    // "BLK0" + blockheight + payloadSize
    auto payloadSize = static_cast<uint32_t>(data.size() - (4U + 4U + 4U));
    std::memcpy(data.data() + (4U + 4U), &payloadSize, 4U);

    return data;
}

[[nodiscard]] auto ChangesInBlock::blockHeight() const noexcept -> uint32_t {
    return mBlockHeight;
}

[[nodiscard]] auto ChangesInBlock::satoshiAndBlockheights() const noexcept -> std::vector<SatoshiAndBlockheight> const& {
    return mSatoshiAndBlockheights;
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

    if (header.magicMarker != uint32_t(0x014b4c42)) {
        throw std::runtime_error("Decoding error, 'BLK\\1' does not match");
    }

    return std::make_pair(header, ptr + sizeof(Header));
}

} // namespace

auto ChangesInBlock::skip(char const* ptr) -> std::pair<uint32_t, char const*> {
    auto [header, payloadPtr] = parseHeader(ptr);
    return std::make_pair(header.blockHeight, payloadPtr + header.numBytes);
}

[[nodiscard]] auto ChangesInBlock::operator==(ChangesInBlock const& other) const noexcept -> bool {
    return mBlockHeight == other.mBlockHeight && mSatoshiAndBlockheights == other.mSatoshiAndBlockheights &&
           mIsFinalized == other.mIsFinalized;
}

[[nodiscard]] auto ChangesInBlock::operator!=(ChangesInBlock const& other) const noexcept -> bool {
    return !(*this == other);
}
auto ChangesInBlock::decode(char const* ptr) -> std::pair<ChangesInBlock, char const*> {
    auto [header, payloadPtr] = parseHeader(ptr);
    auto cib = ChangesInBlock();

    cib.mBlockHeight = header.blockHeight;

    const auto* endPtr = payloadPtr + header.numBytes;

    auto satoshi = uint64_t();
    auto blockHeight = int64_t();
    while (payloadPtr < endPtr) {
        auto diff = SatoshiAndBlockheight(0, 0);
        auto diffSatoshi = uint64_t();
        auto diffBlockheight = int64_t();
        std::tie(diffSatoshi, payloadPtr) = util::VarInt::decode<uint64_t>(payloadPtr);
        std::tie(diffBlockheight, payloadPtr) = util::VarInt::decode<int64_t>(payloadPtr);
        satoshi += diffSatoshi;
        blockHeight += diffBlockheight;
        cib.mSatoshiAndBlockheights.emplace_back(satoshi, blockHeight);
    }

    return std::make_pair(cib, payloadPtr);
}

} // namespace buv

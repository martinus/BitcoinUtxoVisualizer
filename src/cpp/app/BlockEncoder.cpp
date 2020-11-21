#include "BlockEncoder.h"

#include <util/BinaryStreamWriter.h>
#include <util/VarInt.h>

#include <fmt/format.h>

namespace buv {

ChangeAtBlockheight::ChangeAtBlockheight(int64_t satoshi, uint32_t blockHeight)
    : mSatoshi(satoshi)
    , mBlockHeight(blockHeight) {}

[[nodiscard]] auto ChangeAtBlockheight::satoshi() const noexcept -> int64_t {
    return mSatoshi;
}

[[nodiscard]] auto ChangeAtBlockheight::blockHeight() const noexcept -> uint32_t {
    return mBlockHeight;
}

[[nodiscard]] auto ChangeAtBlockheight::operator<(ChangeAtBlockheight const& other) const noexcept -> bool {
    if (mSatoshi != other.mSatoshi) {
        return mSatoshi < other.mSatoshi;
    }
    return mBlockHeight < other.mBlockHeight;
}

[[nodiscard]] auto ChangeAtBlockheight::operator==(ChangeAtBlockheight const& other) const noexcept -> bool {
    return mSatoshi == other.mSatoshi && mBlockHeight == other.mBlockHeight;
}

[[nodiscard]] auto ChangeAtBlockheight::operator!=(ChangeAtBlockheight const& other) const noexcept -> bool {
    return !(*this == other);
}

////

void ChangesInBlock::beginBlock(uint32_t blockHeight) {
    mIsFinalized = false;
    mBlockHeight = blockHeight;
    mChangeAtBlockheights.clear();
}

void ChangesInBlock::finalizeBlock() {
    if (mIsFinalized) {
        throw std::runtime_error("finalizeBlock() has already been called, not needed any more");
    }
    std::sort(mChangeAtBlockheights.begin(), mChangeAtBlockheights.end());
    mIsFinalized = true;
}

void ChangesInBlock::addChange(int64_t satoshi, uint32_t blockHeight) {
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

    data += std::string_view("BLK\x01");
    data.append(reinterpret_cast<char const*>(&mBlockHeight), sizeof(mBlockHeight));

    // skip 4 bytes, which will later contain the size of the remaining payload. This can be used to quickly skip to the next
    // block.
    data.append(4, '\0');

    if (!mChangeAtBlockheights.empty()) {
        // now comes the data in mChangeAtBlockheight. Sorted by satoshi, so the satoshi's only go up.
        // The first entry will probably be negative, if anny old amount was spent. That is encoded as var_int.
        //
        // We only store the difference to the previous satoshi amount for the following entries, encoded as unsigned varint.
        // Thus, the amounts will be stored quite compact. Only thing better would be golomb coded sets
        // https://en.wikipedia.org/wiki/Golomb_coding
        //
        // We also store diffs of block size, but they will be encoded as signed integers, because they can be quite random.
        // already sorted in finishBlock()
        auto varIntEncoder = util::VarInt();

        // TODO(martinus) encode first amount correctly
        auto it = mChangeAtBlockheights.begin();
        data += varIntEncoder.encode<int64_t>(it->satoshi());
        data += varIntEncoder.encode<uint64_t>(it->blockHeight());

        auto pre = it;
        ++it;

        while (it != mChangeAtBlockheights.end()) {
            // the amount diff will always be positive since its sorted, so we can serialize an uint
            data += varIntEncoder.encode<uint64_t>(it->satoshi() - pre->satoshi());
            // diff of blockheight can be negative as well
            data +=
                varIntEncoder.encode<int64_t>(static_cast<int64_t>(it->blockHeight()) - static_cast<int64_t>(pre->blockHeight()));

            pre = it;
            ++it;
        }

        // finally, fill in the payload size
        // "BLK0" + blockheight + payloadSize
        auto payloadSize = static_cast<uint32_t>(data.size() - (4U + 4U + 4U));
        std::memcpy(data.data() + (4U + 4U), &payloadSize, 4U);
    }

    return data;
}

[[nodiscard]] auto ChangesInBlock::blockHeight() const noexcept -> uint32_t {
    return mBlockHeight;
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
    return mBlockHeight == other.mBlockHeight && mChangeAtBlockheights == other.mChangeAtBlockheights &&
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

    auto satoshi = int64_t();
    auto blockHeight = int64_t();
    std::tie(satoshi, payloadPtr) = util::VarInt::decode<int64_t>(payloadPtr);
    std::tie(blockHeight, payloadPtr) = util::VarInt::decode<uint64_t>(payloadPtr);
    cib.mChangeAtBlockheights.emplace_back(satoshi, blockHeight);

    while (payloadPtr < endPtr) {
        auto diffSatoshi = uint64_t();
        auto diffBlockheight = int64_t();
        std::tie(diffSatoshi, payloadPtr) = util::VarInt::decode<uint64_t>(payloadPtr);
        std::tie(diffBlockheight, payloadPtr) = util::VarInt::decode<int64_t>(payloadPtr);
        satoshi += diffSatoshi;
        blockHeight += diffBlockheight;
        cib.mChangeAtBlockheights.emplace_back(satoshi, blockHeight);
    }

    return std::make_pair(cib, payloadPtr);
}

} // namespace buv

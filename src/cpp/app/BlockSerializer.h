#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace buv {

// Serializes a block into a string
class BlockSerializer {
    std::vector<std::pair<uint64_t, uint32_t>> mSatoshiAndBlockheight{};
    uint32_t mBlockHeight = 0;

public:
    void beginBlock(uint32_t blockHeight);
    void addSpentOutput(uint32_t blockHeight, uint64_t amountSatoshi);
    void finishBlock();

    // serializes everything into data. Non-const so we can sort mSatoshiAndBlockheight
    void serialize(std::string& data) const;
};

} // namespace buv

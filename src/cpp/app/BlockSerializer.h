#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

namespace buv {

// Serializes change data
class BlockSerializer {
    std::ofstream mOut{};
    uint32_t mCurrentBlockHeight{};
    std::vector<std::pair<uint64_t, uint32_t>> mSatoshiAndBlockheight{};
    std::string mTmp{};

public:
    explicit BlockSerializer(std::filesystem::path const& filename);

    void beginBlock(uint32_t blockHeight);
    void add(uint32_t blockHeight, uint64_t amountSatoshi);
    void finishBlock();
};

} // namespace buv

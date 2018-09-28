#include "Change.h"

#include <chrono>
#include <cstdint>
#include <iostream>

class Result
{
public:
    void begin_block(uint32_t block_height)
    {
        ++mLastBlock;
        if (mLastBlock != block_height) {
            std::cout << "got " << block_height << " expected " << mLastBlock << std::endl;
            mLastBlock = block_height;
        }
    }

    void change(uint32_t height, int64_t amount)
    {
        ++mNumChanges;
        mSum += amount;
        mHeights += height;
    }

    void end_block()
    {
        std::cout << "block done!" << std::endl;
    }

    int64_t sum() const
    {
        return mSum;
    }

    uint64_t changes() const
    {
        return mNumChanges;
    }

    uint32_t lastBlock() const
    {
        return mLastBlock;
    }

private:
    uint32_t mLastBlock = -1;
    int64_t mSum = 0;
    uint64_t mNumChanges = 0;
    uint32_t mHeights = 0;
};


template <class T>
double dur(T before)
{
    return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - before).count();
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "usage: bv input.bin" << std::endl;
        return 1;
    }


    std::string filename = argv[1];
    auto t = std::chrono::high_resolution_clock::now();
    Result r;
    bool isOk = bv::parse_change_data_v2(filename.c_str(), r);
    auto duration = dur(t);
    std::cout << duration << " sec. changes: " << r.changes() << ", sum=" << r.sum() << ", last block=" << r.lastBlock() << std::endl;
    std::cout << (r.changes() / (dur(t) * 1000'000)) << "M changes per second" << std::endl;

    std::cout << "ok? " << (isOk ? "YES" : "NO") << std::endl;
}
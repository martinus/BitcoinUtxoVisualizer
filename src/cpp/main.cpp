#include "Change.h"

#include <chrono>
#include <cstdint>
#include <iostream>

class Result
{
public:
    void begin_block(uint32_t block_height)
    {
        ++mNumBlocks;
    }

    void change(uint32_t height, int64_t amount)
    {
        mSum += amount;

        mHeights += height;
    }

    void end_block()
    {
        std::cout << "block done!" << std::endl;
    }

    size_t numBlocks() const
    {
        return mNumBlocks;
    }

    int64_t sum() const
    {
        return mSum;
    }

private:
    int64_t mSum = 0;
    uint32_t mHeights = 0;
    size_t mNumBlocks = 0;
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
    bv::parse_change_data_v2(filename.c_str(), r);
    std::cout << dur(t) << " sec for " << r.numBlocks() << " blocks. sum=" << r.sum() << std::endl;
}
#pragma once

#include <fstream>

namespace bv {


template <typename S, typename T>
void read(S& stream, T& obj)
{
    stream.read(reinterpret_cast<char*>(&obj), sizeof(T));
}

template <class T>
bool parse_change_data(const char* filename, T& result)
{
    std::ifstream fin(filename, std::ios::binary);
    if (!fin.is_open()) {
        return false;
    }

    uint32_t block_height;
    uint32_t num_changes;
    int64_t amount;

    while (true) {
        read(fin, block_height);
        if (fin.eof()) {
            break;
        }
        read(fin, num_changes);
        result.begin_block(block_height, num_changes);
        for (uint32_t i = 0; i < num_changes; ++i) {
            read(fin, block_height);
            read(fin, amount);
            result.change(block_height, amount);
        }
    }

    return true;
}


} // namespace bv
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
        result.begin_block(block_height);
        for (uint32_t i = 0; i < num_changes; ++i) {
            read(fin, block_height);
            read(fin, amount);
            result.change(block_height, amount);
        }
    }

    return true;
}

template <typename T>
size_t decode_uint(std::istream& is, T& val)
{
    int shift = 0;
    int num_bytes = 0;
    T x = 0;
    while (true) {
        uint8_t byte;
        is.read(reinterpret_cast<char*>(&byte), 1);
        x |= static_cast<T>(byte & 0b01111111) << shift;
        ++num_bytes;
        shift += 7;
        if (0 == (byte & 0b10000000)) {
            break;
        }
    }
    val = x;
    return num_bytes;
}

template <typename T>
size_t decode_int(std::istream& is, T& val)
{
    T v;
    auto num_bytes = decode_uint(is, v);
    val = static_cast<T>((v >> 1) ^ -(v & 1));
    return num_bytes;
}

template <class T>
bool parse_change_data_v2(const char* filename, T& result)
{
    std::ifstream fin(filename, std::ios::binary);
    if (!fin.is_open()) {
        return false;
    }

    while (true) {
        uint32_t magick_BLK0;
        read(fin, magick_BLK0);
        if (fin.eof()) {
            break;
        }

        if (0x004b4c42 != magick_BLK0) {
            return false;
        }

        uint32_t block_height;
        read(fin, block_height);
        result.begin_block(block_height);

        uint32_t num_bytes_total;
        read(fin, num_bytes_total);

        // read first dataset
        int64_t amount;
        read(fin, amount);
        read(fin, block_height);
        result.change(block_height, amount);

        size_t bytes_read = sizeof(amount) + sizeof(block_height);

        while (bytes_read < num_bytes_total) {
            uint64_t amount_diff;
            bytes_read += decode_uint(fin, amount_diff);
            amount += amount_diff;

            int32_t block_height_diff;
            bytes_read += decode_int(fin, block_height_diff);
            block_height += block_height_diff;
            result.change(block_height, amount);
        }
    }

    return true;
}

} // namespace bv
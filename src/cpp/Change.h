#pragma once

#include "BufferedStreamReader.h"

namespace bv {

template <typename S, typename T>
void read(S& stream, T& obj)
{
    stream.read<sizeof(T)>(reinterpret_cast<char*>(&obj));
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

    BufferedStreamReader bsr(fin);

    while (true) {
        read(bsr, block_height);
        if (bsr.eof()) {
            break;
        }
        read(bsr, num_changes);
        result.begin_block(block_height);
        for (uint32_t i = 0; i < num_changes; ++i) {
            read(bsr, block_height);
            read(bsr, amount);
            result.change(block_height, amount);
        }
    }

    return true;
}

template <typename S, typename T>
size_t decode_uint(S& is, T& val)
{
    int num_bytes = 0;
    val = 0;
    uint8_t byte;
    do {
        is.read1(reinterpret_cast<char*>(&byte));
        val |= static_cast<T>(byte & 0b01111111) << (7 * num_bytes);
        ++num_bytes;
    } while (byte > 0b01111111);
    return num_bytes;
}

template <typename S, typename T>
size_t decode_int(S& is, T& val)
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

    BufferedStreamReader<> bsr(fin);

    while (true) {
        uint32_t magick_BLK0;
        read(bsr, magick_BLK0);
        if (bsr.eof()) {
            break;
        }

        if (0x004b4c42 != magick_BLK0) {
            return false;
        }

        uint32_t block_height;
        read(bsr, block_height);
        result.begin_block(block_height);

        uint32_t num_bytes_total;
        read(bsr, num_bytes_total);

        // read first dataset
        int64_t amount;
        read(bsr, amount);
        read(bsr, block_height);
        result.change(block_height, amount);

        size_t bytes_read = sizeof(amount) + sizeof(block_height);

        while (bytes_read < num_bytes_total) {
            uint64_t amount_diff;
            bytes_read += decode_uint(bsr, amount_diff);
            amount += amount_diff;

            int32_t block_height_diff;
            bytes_read += decode_int(bsr, block_height_diff);
            block_height += block_height_diff;
            result.change(block_height, amount);
        }
    }

    return true;
}

} // namespace bv
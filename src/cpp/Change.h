#pragma once

#include "BufferedStreamReader.h"

namespace bv {

template <typename S, typename T>
size_t decode_uint(S& is, T& val)
{
    static_assert(std::is_unsigned<T>::value, "only for unsigned types");

    int num_bytes = 0;
    val = 0;
    uint8_t byte;
    is.read1(byte);
    while (byte & 0b10000000) {
        val |= static_cast<T>(byte & 0b01111111) << (7 * num_bytes);
        ++num_bytes;
        is.read1(byte);
    }
    val |= static_cast<T>(byte) << (7 * num_bytes);
    return num_bytes + 1;
}

template <typename S>
size_t decode_int32(S& is, int32_t& val)
{
    uint32_t v;
    auto num_bytes = decode_uint(is, v);
    val = static_cast<int32_t>((v >> 1) ^ (uint32_t)0 - (v & 1));
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
        bsr.read(magick_BLK0);
        if (bsr.eof()) {
            break;
        }

        if (0x004b4c42 != magick_BLK0) {
            return false;
        }

        uint32_t current_block_height;
        bsr.read(current_block_height);
        result.begin_block(current_block_height);

        uint32_t num_bytes_total;
        bsr.read(num_bytes_total);

        // read first dataset
        int64_t amount;
        uint32_t amount_block_height;
        bsr.read(amount);
        bsr.read(amount_block_height);
        result.change(amount_block_height, amount);

        size_t bytes_read = sizeof(amount) + sizeof(amount_block_height);

        while (bytes_read < num_bytes_total) {
            uint64_t amount_diff;
            bytes_read += decode_uint(bsr, amount_diff);
            amount += amount_diff;

            int32_t block_height_diff;
            bytes_read += decode_int32(bsr, block_height_diff);
            amount_block_height += block_height_diff;
            result.change(amount_block_height, amount);
        }

        result.end_block(current_block_height);
    }

    return true;
}

} // namespace bv
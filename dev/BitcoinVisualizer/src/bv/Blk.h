#pragma once

#include <bv/BufferedStreamReader.h>

namespace bv {

// Decodes a .blk file, and calls the given callbacks for each event.
//
// BLK is designed to be compact, fast to parse, and contain all (amounts, blockheight) tuples of the current block.
// a .blk file contains concatenated entries of that payload:
//
// field size | description  | data type | commment
// -----------|--------------|-----------|---
//          4 | marker       | string    | magic marker "BLK\0". uint32_t with value 0x004b4c42. Always exactly the same.
//          4 | block height | uint32_t  | current block height, successively increasing value
//          4 | num_bytes    | uint32_t  | size in bytes of the data blob for this blob. Add num_bytes to skip to the next block, then you will point to the marker "BLK\0" of the next block. This is useful to quickly skip to the next block without parsing the data.
//          8 | amount       | int64_t   | Amount in satishi of the first transaction (transaction with the lowest amount)
//          4 | amount_block | uint32_t  | Block height of the first amount
//  The following fields are repeated until the end of the block (num_bytes - (8 + 4) bytes). Sorted by amount.
//         1+ | amount_diff  | var_uint  | var-uint encoded difference to previous amount. Guaranteed to be positive due to the sorting.
//         1+ | block_diff   | var_int   | var-int (zig-zag encoding) of block difference to previous entry.
class Blk
{
public:
    template <class T>
    static bool decode(std::string filename, T& callback, uint32_t* last_block_height)
    {
        std::ifstream fin(filename, std::ios::binary);
        if (!fin.is_open()) {
            return false;
        }

        BufferedStreamReader<> bsr(fin);

        uint32_t current_block_height;
        while (true) {
            uint32_t magick_BLK0;
            bsr.read(magick_BLK0);
            if (bsr.eof()) {
                if (last_block_height) {
                    *last_block_height = current_block_height;
                }
                break;
            }

            // "BLK\0"
            if (0x004b4c42 != magick_BLK0) {
                return false;
            }

            bsr.read(current_block_height);
            callback.begin_block(current_block_height);

            uint32_t num_bytes_total;
            bsr.read(num_bytes_total);

            int64_t amount;
            uint32_t amount_block_height;

            // read first dataset: not varint encoded
            bsr.read(amount);
            bsr.read(amount_block_height);
            callback.change(amount_block_height, amount, false);

            size_t bytes_read = sizeof(amount) + sizeof(amount_block_height);

            while (bytes_read < num_bytes_total) {
                uint64_t amount_diff;
                bytes_read += decode_uint(bsr, amount_diff);
                amount += amount_diff;

                int32_t block_height_diff;
                bytes_read += decode_int32(bsr, block_height_diff);
                amount_block_height += block_height_diff;
                callback.change(amount_block_height, amount, amount_diff == 0 && block_height_diff == 0);
                //callback.change(amount_block_height, amount, false);
            }

            callback.end_block(current_block_height);
        }

        return true;
    }

private:
    template <typename S, typename T>
    static size_t decode_uint(S& is, T& val)
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
    static size_t decode_int32(S& is, int32_t& val)
    {
        uint32_t v;
        auto num_bytes = decode_uint(is, v);
        val = static_cast<int32_t>((v >> 1) ^ (uint32_t)0 - (v & 1));
        return num_bytes;
    }
};


} // namespace bv
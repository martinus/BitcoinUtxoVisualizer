#include "Blk.h"
#include "ColorMap.h"
#include "Density.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <unordered_set>

#include <intrin.h>

// check if all blocks are in sequential order
struct CheckSequential {
    void begin_block(uint32_t block_height)
    {
        if (block_height != m_last_block + 1) {
            std::cout << "begin_block: expected block " << (m_last_block + 1) << " but got " << block_height << std::endl;
        }
        m_last_block = block_height;
    }

    void change(uint32_t, int64_t)
    {
        ++m_num_changes;
    }

    void end_block(uint32_t block_height)
    {
        if (block_height != m_last_block) {
            std::cout << "end_block: expected block " << m_last_block << " but got " << block_height << std::endl;
        }
    }

    uint32_t m_last_block = -1;
    uint64_t m_num_changes = 0;
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


    size_t const density_per_pixel = static_cast<size_t>(1000) * 3840 * 2160;
    size_t const width = 3840;
    size_t const height = 2160;
    size_t const max_included_density = 800;

    bv::Density density(
        width,                   // width
        height,                  // height
        1,                       // minimum satoshi
        10'000ULL * 100'000'000, // max satoshi,
        0,                       // minimum block height
        550'000,                 // maximum block height
        max_included_density,    // max included density value for colorization
        bv::ColorMap::viridis()  // colorization type
    );
    bool isOk = bv::Blk::decode(filename, density);
    density.save_image_ppm("final.ppm");

    auto duration = dur(t);
    std::cout << "done in " << duration << " seconds." << std::endl;
    std::cout << "Parsing ok? " << (isOk ? "YES" : "NO") << std::endl;


    /*
    CheckSequential r;
    bool isOk = bv::parse_change_data(filename.c_str(), r);
    auto duration = dur(t);
    std::cout << "done in " << duration << " seconds." << std::endl;
    std::cout << "Parsing ok? " << (isOk ? "YES" : "NO") << std::endl;

    std::cout << "m_num_changes=" << r.m_num_changes << std::endl;
    std::cout << (r.m_num_changes / (duration * 1000'000)) << "M changes per second" << std::endl;
	*/

    /*
    std::cout << r.m_num_outputs_created << " outputs created (" << (r.m_num_outputs_created * 1.0 / r.m_num_blocks) << " per block)" << std::endl;
    std::cout << r.m_num_outputs_destroyed << " outputs destroyed (" << (r.m_num_outputs_destroyed * 1.0 / r.m_num_blocks) << " per block)" << std::endl;
    std::cout << (r.m_num_outputs_created - r.m_num_outputs_destroyed) << " outputs remain" << std::endl;

    std::cout << r.m_min_block << " min block" << std::endl;
    std::cout << r.m_max_block << " max block" << std::endl;
	*/
}
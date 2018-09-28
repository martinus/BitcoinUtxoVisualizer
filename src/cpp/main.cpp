#include "Change.h"

#include <chrono>
#include <cstdint>
#include <iostream>

struct AverageNumberOfOutputs {
    void begin_block(uint32_t block_height)
    {
        if (block_height > m_max_block) {
            m_max_block = block_height;
        }
        if (block_height < m_min_block) {
            m_min_block = block_height;
        }
    }

    void change(uint32_t height, int64_t amount)
    {
        if (amount >= 0) {
            ++m_num_outputs_created;
        } else {
            ++m_num_outputs_destroyed;
        }
    }

    void end_block(uint32_t block_height)
    {
        ++m_num_blocks;
    }

    uint32_t m_min_block = std::numeric_limits<uint32_t>::max();
    uint32_t m_max_block = 0;

    uint64_t m_num_outputs_created = 0;
    uint64_t m_num_outputs_destroyed = 0;
    uint64_t m_num_blocks = 0;
};

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
    CheckSequential r;
    bool isOk = bv::parse_change_data_v2(filename.c_str(), r);
    std::cout << "done in " << dur(t) << " seconds." << std::endl;
    std::cout << "Parsing ok? " << (isOk ? "YES" : "NO") << std::endl;

    std::cout << "m_num_changes=" << r.m_num_changes << std::endl;

    /*
    std::cout << r.m_num_outputs_created << " outputs created (" << (r.m_num_outputs_created * 1.0 / r.m_num_blocks) << " per block)" << std::endl;
    std::cout << r.m_num_outputs_destroyed << " outputs destroyed (" << (r.m_num_outputs_destroyed * 1.0 / r.m_num_blocks) << " per block)" << std::endl;
    std::cout << (r.m_num_outputs_created - r.m_num_outputs_destroyed) << " outputs remain" << std::endl;

    std::cout << r.m_min_block << " min block" << std::endl;
    std::cout << r.m_max_block << " max block" << std::endl;
	*/
}
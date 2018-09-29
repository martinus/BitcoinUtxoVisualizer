#pragma once

#include <array>
#include <fstream>

namespace bv {

// unfortunately, reading byte by byte from an std::ifstream is incredibly slow, as each
// call to read() has a virtual call overhead. Using this class as a wrapper speeds up
// the parsing from about 5M to 80M - 16 times faster.
template <size_t BufferSize = 32 * 1024>
class BufferedStreamReader
{
public:
    BufferedStreamReader(std::ifstream& in)
        : mIn(&in)
    {
    }

    // templated size_t can be a bit faster than normal read() because the compiler could
    // inline it.
    template <typename T>
    void read(T& target_blob)
    {
        for (size_t i = 0; i < sizeof(T); ++i) {
            if (mIdx == BufferSize && !fetch()) {
                return;
            }
            reinterpret_cast<char*>(&target_blob)[i] = mBuf[mIdx++];
        }
    }

    // the compiler is not smart enough to optimize the read<1> call as much as this hand written
    // code.
    template <typename T>
    void read1(T& target_blob)
    {
        if (mIdx == BufferSize && !fetch()) {
            return;
        }
        *reinterpret_cast<char*>(&target_blob) = mBuf[mIdx++];
    }

    bool eof() const
    {
        return mIdx == BufferSize && mIn->eof();
    }

private:
    // slow path: only called when we need new data from fstream.
    // We have plenty of time here, just make sure the read() methods are as fast as possible.
    //
    // We make this noinline so read() can more easily be inlined, and if we have read less
    // than requested (usually only happens at the end of the file), we move the rest to the
    // end of the buffer so read() can only check against the compile time constant BufferSize.
    bool fetch()
    {
        mIn->read(mBuf.data(), BufferSize);
        size_t const num_bytes_read = static_cast<size_t>(mIn->gcount());
        if (num_bytes_read != BufferSize) {
            if (0 == num_bytes_read) {
                return false;
            }

            mIdx = BufferSize - num_bytes_read;
            std::memmove(mBuf.data() + mIdx, mBuf.data(), num_bytes_read);
            return true;
        }
        mIdx = 0;
        return true;
    }

    std::array<char, BufferSize> mBuf;
    std::istream* mIn;
    size_t mIdx = BufferSize;
};

} // namespace bv
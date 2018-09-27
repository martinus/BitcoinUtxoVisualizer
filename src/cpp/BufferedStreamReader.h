#pragma once

#include <array>
#include <fstream>

namespace bv {

// unfortunately, reading byte by byte from an std::ifstream is incredibly slow, as each
// call to read() has a virtual call overhead. Using this class as a wrapper speeds up
// the parsing from about 5M to 80M - 16 times faster.
template <size_t BufferSize = 16 * 1024>
class BufferedStreamReader
{
public:
    BufferedStreamReader(std::ifstream& in)
        : mIn(&in)
    {
    }

    inline void read(char* target, size_t num_bytes)
    {
        for (size_t i = 0; i < num_bytes; ++i) {
            if (mIdx == BufferSize && !fetch()) {
                return;
            }
            target[i] = mBuf[mIdx++];
        }
    }

    template <size_t NumBytes>
    inline void read(char* target)
    {
        for (size_t i = 0; i < NumBytes; ++i) {
            if (mIdx == BufferSize && !fetch()) {
                return;
            }
            target[i] = mBuf[mIdx++];
        }
    }

	// the compiler is not smart enough to optimize the read<1> call as much as the read1.
    inline void read1(char* target)
    {
        if (mIdx == BufferSize && !fetch()) {
            return;
        }
        *target = mBuf[mIdx++];
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
    __declspec(noinline) bool fetch()
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
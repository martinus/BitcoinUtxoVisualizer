#include "Mmap.h"

#include <exception>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace util {

// See https://techoverflow.net/2013/08/21/a-simple-mmap-readonly-example/
Mmap::Mmap(std::filesystem::path const& f) {
    auto ec = std::error_code();
    mSize = std::filesystem::file_size(f, ec);
    if (ec) {
        // could not determine file size
        return;
    }

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    mFileDescriptor = open(f.c_str(), O_RDONLY | O_CLOEXEC, 0);
    if (mFileDescriptor == -1 || mSize <= 0) {
        mFileDescriptor = -1;
        // could not open file
        return;
    }

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    mData = ::mmap(nullptr, mSize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, mFileDescriptor, 0);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    if (mData == MAP_FAILED) {
        // mmap failed
        ::close(mFileDescriptor);
        mData = nullptr;
        mFileDescriptor = -1;
    }
}

void Mmap::close() noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    if (mFileDescriptor != -1 && mData != MAP_FAILED && mData != nullptr) {
        ::munmap(mData, mSize);
        ::close(mFileDescriptor);
    }
}

Mmap::~Mmap() {
    close();
}

Mmap::Mmap(Mmap&& other) noexcept
    : mData(std::exchange(other.mData, nullptr))
    , mSize(std::exchange(other.mSize, 0U))
    , mFileDescriptor(std::exchange(other.mFileDescriptor, -1)) {}

auto Mmap::operator=(Mmap&& other) noexcept -> Mmap& {
    if (this != &other) {
        close();
        mData = std::exchange(other.mData, nullptr);
        mSize = std::exchange(other.mSize, 0U);
        mFileDescriptor = std::exchange(other.mFileDescriptor, -1);
    }
    return *this;
}

auto Mmap::data() const -> char const* {
    return static_cast<char const*>(mData);
}

auto Mmap::size() const -> size_t {
    return mSize;
}

auto Mmap::begin() const -> char const* {
    return static_cast<char const*>(mData);
}

auto Mmap::end() const -> char const* {
    return static_cast<char const*>(mData) + mSize;
}


auto Mmap::view() const -> std::string_view {
    return std::string_view(static_cast<char const*>(mData), mSize);
}

auto Mmap::is_open() const -> bool {
    return mFileDescriptor != -1;
}

} // namespace util

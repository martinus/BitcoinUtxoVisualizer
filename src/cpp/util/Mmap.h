#pragma once

#include <cstddef>
#include <filesystem>
#include <string_view>

namespace util {

// memory maps a file for read only access
class Mmap {
    void* mData = nullptr;
    size_t mSize = 0;
    int mFileDescriptor = -1;

public:
    // if pool is not specified, it allocates one itself.
    explicit Mmap(std::filesystem::path const& filename);

    // no copy is allowed
    Mmap(Mmap const&) = delete;
    auto operator=(Mmap const&) -> Mmap& = delete;

    // move is allowed
    Mmap(Mmap&&) noexcept;
    auto operator=(Mmap&&) noexcept -> Mmap&;

    [[nodiscard]] auto is_open() const -> bool;

    ~Mmap();

    [[nodiscard]] auto data() const -> void*;

    // size in bytes
    [[nodiscard]] auto size() const -> size_t;

    // Gets a std::string_view of the whole mmaped file
    [[nodiscard]] auto view() const -> std::string_view;

private:
    void close() noexcept;
};

} // namespace util

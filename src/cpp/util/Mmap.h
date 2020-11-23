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
    ~Mmap();

    // if pool is not specified, it allocates one itself.
    explicit Mmap(std::filesystem::path const& filename);

    // no copy is allowed
    Mmap(Mmap const&) = delete;
    auto operator=(Mmap const&) -> Mmap& = delete;

    // move is allowed
    Mmap(Mmap&&) noexcept;
    auto operator=(Mmap&&) noexcept -> Mmap&;

    [[nodiscard]] auto is_open() const -> bool;

    [[nodiscard]] auto data() const -> char const*;
    [[nodiscard]] auto size() const -> size_t;

    [[nodiscard]] auto begin() const -> char const*;
    [[nodiscard]] auto end() const -> char const*;

    // Gets a std::string_view of the whole mmaped file
    [[nodiscard]] auto view() const -> std::string_view;

private:
    void close() noexcept;
};

} // namespace util

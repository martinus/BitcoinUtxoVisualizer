#pragma once

#include <cstdint>
#include <cstring> // std::memcpy
#include <stdexcept>
#include <type_traits>

namespace util {

// Helper to read binary data from a sequential chunk of memory
class BinaryStreamReader {
    uint8_t const* mNow = nullptr;
    uint8_t const* mEnd = nullptr;

public:
    inline explicit BinaryStreamReader(void* data, size_t size)
        : mNow(reinterpret_cast<uint8_t const*>(data))
        , mEnd(mNow + size) {}

    // writes any binary blob. Makes sure it is actually serializable trivially
    template <int ExpectedSize, typename T>
    auto read() -> T {
        static_assert(ExpectedSize == sizeof(T), "make sure we serialize exactly as many bytes as we expect");
        static_assert(std::is_standard_layout_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);

        if (mNow + ExpectedSize > mEnd) {
            throw std::out_of_range("BinaryStreamReader: deserialization error, out of range!");
        }

        // t might be uninitialized, but that's ok since we memcpy
        T t;
        std::memcpy(&t, mNow, ExpectedSize);
        return t;
    }
};

} // namespace util

#pragma once

#include <array>
#include <ostream>
#include <type_traits>

namespace util {

// Helper to dump binary data into an ostream
class BinaryStreamWriter {
    std::ostream* mStream = nullptr;

public:
    inline explicit BinaryStreamWriter(std::ostream* stream)
        : mStream(stream) {}

    template <int ExpectedSize, size_t ArraySize>
    void write(char const (&a)[ArraySize]) {
        static_assert(ExpectedSize == ArraySize - 1);
        mStream->write(a, ExpectedSize);
    }

    // writes any binary blob. Makes sure it is actually serializable trivially
    template <int ExpectedSize, typename T, typename = std::enable_if_t<!std::is_array_v<T>>>
    void write(T const& t) {
        static_assert(ExpectedSize == sizeof(T), "make sure we serialize exactly as many bytes as we expect");
        static_assert(std::is_standard_layout_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);

        mStream->write(reinterpret_cast<char const*>(&t), sizeof(T));
    }
};

} // namespace util

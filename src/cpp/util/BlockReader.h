#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace util {

// all hidden in cpp because compile time of httplib is abmyssal
class BlockReader {
public:
    // reads a block
    [[nodiscard]] virtual auto read(std::string_view blockId) -> std::string = 0;

    virtual ~BlockReader() = default;
    BlockReader() = default;
    
    BlockReader(BlockReader const&) = delete;
    BlockReader(BlockReader&&) = delete;
    auto operator=(BlockReader const&) -> BlockReader& = delete;
    auto operator=(BlockReader &&) -> BlockReader& = delete;

    static auto create(char const* schemeHostPort) -> std::unique_ptr<BlockReader>;
};

} // namespace util

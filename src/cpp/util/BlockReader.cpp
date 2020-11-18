#include "BlockReader.h"

#include <httplib.h>

namespace util {

class BlockReaderImpl : public BlockReader {
    httplib::Client mClient;

public:
    explicit BlockReaderImpl(char const* schemeHostPort)
        : BlockReader()
        , mClient(schemeHostPort) {}

    [[nodiscard]] auto read(std::string_view blockId) -> std::string override {
        auto where = std::string("/rest/block/") + std::string(blockId) + ".json";
        auto res = mClient.Get(where.c_str());
        if (res->status != 200) {
            throw std::runtime_error("BlockReader: could not get");
        }
        return std::move(res->body);
    }
};

auto BlockReader::create(char const* schemeHostPort) -> std::unique_ptr<BlockReader> {
    return std::make_unique<BlockReaderImpl>(schemeHostPort);
}

} // namespace util
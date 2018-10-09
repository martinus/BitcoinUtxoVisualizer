#pragma once

#include <cstdint>
#include <memory>

namespace bv {

// abstract class so we can hide all the nasty OS specific stuff in the .cpp file.
class SocketStream
{
public:
    virtual void write(const char* data, size_t size) = 0;
    virtual ~SocketStream() = default;

    // factory
    static std::unique_ptr<SocketStream> create(const char* ip_addr, uint16_t socket);
};


} // namespace bv
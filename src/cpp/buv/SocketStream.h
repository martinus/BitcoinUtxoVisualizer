#pragma once

#include <cstdint>
#include <memory>

namespace buv {

// abstract class so we can hide all the nasty OS specific stuff in the .cpp file.
// Connects to a server and writes data to that socket.
class SocketStream {
public:
    SocketStream() = default;
    virtual void write(uint8_t const* data, size_t size) = 0;
    virtual ~SocketStream() = default;

    SocketStream(SocketStream const&) = delete;
    SocketStream(SocketStream&&) = delete;
    auto operator=(SocketStream const&) -> SocketStream& = delete;
    auto operator=(SocketStream&&) -> SocketStream& = delete;

    // factory
    static auto create(const char* ip_addr, uint16_t socket) -> std::unique_ptr<SocketStream>;
};

} // namespace buv

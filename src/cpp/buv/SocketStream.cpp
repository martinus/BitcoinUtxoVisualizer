#include "SocketStream.h"

#ifdef _WIN32

#    include <winsock2.h>

namespace buv {

class SocketStreamImpl final : public SocketStream {
public:
    SocketStreamImpl(char const* ip_addr, uint16_t socket_nr) {
        WSAStartup(MAKEWORD(2, 0), &m_wsadata);
        m_socket = socket(AF_INET, SOCK_STREAM, 0);

        m_addr.sin_addr.s_addr = inet_addr(ip_addr);
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(socket_nr);

        connect(m_socket, (SOCKADDR*)&m_addr, sizeof(m_addr));
    }

    ~SocketStreamImpl() {
        closesocket(m_socket);
        WSACleanup();
    }

    void write(uint8_t const* data, size_t size) override {
        send(m_socket, reinterpret_cast<char const*>(data), static_cast<int>(size), 0);
    }

private:
    WSADATA m_wsadata;
    SOCKET m_socket;
    SOCKADDR_IN m_addr;
};

} // namespace buv

#else

#    include <stdexcept>

#    include <arpa/inet.h> // inet_addr
#    include <netinet/in.h> // scockaddr_in
#    include <sys/socket.h> // socket
#    include <unistd.h> // close

namespace buv {

class SocketStreamImpl final : public SocketStream {
    int mSocket = -1;
    sockaddr_in mServAddr{};

public:
    SocketStreamImpl(char const* ip_addr, uint16_t socket_nr)
        : SocketStream() {
        mSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (mSocket < 0) {
            throw std::runtime_error("could not create socket");
        }

        mServAddr = sockaddr_in();
        mServAddr.sin_addr.s_addr = inet_addr(ip_addr);
        mServAddr.sin_family = AF_INET;
        mServAddr.sin_port = htons(socket_nr);
        if (0 != connect(mSocket, reinterpret_cast<sockaddr*>(&mServAddr), sizeof(mServAddr))) {
            throw std::runtime_error("could not connect()");
        }
    }

    ~SocketStreamImpl() override {
        if (mSocket != -1) {
            close(mSocket);
        }
    }

    SocketStreamImpl(SocketStreamImpl const&) = delete;
    SocketStreamImpl(SocketStreamImpl&&) = delete;
    auto operator=(SocketStreamImpl const&) -> SocketStreamImpl& = delete;
    auto operator=(SocketStreamImpl&&) -> SocketStreamImpl& = delete;

    void write(uint8_t const* data, size_t size) override {
        // loop until we've sent everything
        while (size != 0) {
            auto sentBytes = send(mSocket, reinterpret_cast<char const*>(data), static_cast<int>(size), 0);
            if (sentBytes == -1) {
                throw std::runtime_error("send failed!");
            }
            size -= sentBytes;
            data += sentBytes;
        }
    }
};

} // namespace buv

#endif

namespace buv {

auto SocketStream::create(const char* ip_addr, uint16_t socket) -> std::unique_ptr<SocketStream> {
    return std::make_unique<SocketStreamImpl>(ip_addr, socket);
}

} // namespace buv

#include <bv/SocketStream.h>

#include <winsock2.h>

namespace bv {

class SocketStreamImpl final : public SocketStream
{
public:
    SocketStreamImpl(char const* ip_addr, uint16_t socket_nr)
    {
        WSAStartup(MAKEWORD(2, 0), &m_wsadata);
        m_socket = socket(AF_INET, SOCK_STREAM, 0);

        m_addr.sin_addr.s_addr = inet_addr(ip_addr);
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(socket_nr);

        connect(m_socket, (SOCKADDR*)&m_addr, sizeof(m_addr));
    }

    ~SocketStreamImpl()
    {
        closesocket(m_socket);
        WSACleanup();
    }

    void write(uint8_t const* data, size_t size) override
    {
        send(m_socket, reinterpret_cast<char const*>(data), static_cast<int>(size), 0);
    }

private:
    WSADATA m_wsadata;
    SOCKET m_socket;
    SOCKADDR_IN m_addr;
};

std::unique_ptr<SocketStream> SocketStream::create(const char* ip_addr, uint16_t socket)
{
    return std::make_unique<SocketStreamImpl>(ip_addr, socket);
}

} // namespace bv

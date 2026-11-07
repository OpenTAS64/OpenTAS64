#include "TcpClient.h"

#include <array>
#include <cstdint>
#include <cstring>

#if defined(_WIN32)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
#endif

TcpClient::~TcpClient()
{
    Disconnect();
}

bool TcpClient::Connect(const char* host, std::uint16_t port)
{
    Disconnect();
#if defined(_WIN32)
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return false;
    winsockStarted_ = true;
#endif

    const auto candidate = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if defined(_WIN32)
    if (candidate == INVALID_SOCKET)
#else
    if (candidate < 0)
#endif
    {
        Disconnect();
        return false;
    }
    socket_ = static_cast<Socket>(candidate);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &address.sin_addr) != 1 ||
        ::connect(static_cast<decltype(candidate)>(socket_),
                  reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
    {
        Disconnect();
        return false;
    }
    return true;
}

void TcpClient::Disconnect()
{
    if (socket_ != InvalidSocket)
    {
#if defined(_WIN32)
        closesocket(static_cast<SOCKET>(socket_));
#else
        close(socket_);
#endif
        socket_ = InvalidSocket;
    }
#if defined(_WIN32)
    if (winsockStarted_)
    {
        WSACleanup();
        winsockStarted_ = false;
    }
#endif
}

bool TcpClient::IsConnected() const
{
    return socket_ != InvalidSocket;
}

bool TcpClient::SendController(int controller, const ControllerState& c)
{
    if (!IsConnected() || controller < 0 || controller > 3)
        return false;

    std::uint16_t mask = 0;
    if (c.dRight) mask |= 1u << 0;
    if (c.dLeft)  mask |= 1u << 1;
    if (c.dDown)  mask |= 1u << 2;
    if (c.dUp)    mask |= 1u << 3;
    if (c.start)  mask |= 1u << 4;
    if (c.z)      mask |= 1u << 5;
    if (c.b)      mask |= 1u << 6;
    if (c.a)      mask |= 1u << 7;
    if (c.cRight) mask |= 1u << 8;
    if (c.cLeft)  mask |= 1u << 9;
    if (c.cDown)  mask |= 1u << 10;
    if (c.cUp)    mask |= 1u << 11;
    if (c.r)      mask |= 1u << 12;
    if (c.l)      mask |= 1u << 13;

    const std::array<std::uint8_t, 10> packet{
        'O', 'T', '6', '4', 1, static_cast<std::uint8_t>(controller),
        static_cast<std::uint8_t>((mask >> 8) & 0xFF),
        static_cast<std::uint8_t>(mask & 0xFF),
        static_cast<std::uint8_t>(c.stickX),
        static_cast<std::uint8_t>(c.stickY)
    };

#if defined(_WIN32)
    const int sent = send(static_cast<SOCKET>(socket_),
        reinterpret_cast<const char*>(packet.data()),
        static_cast<int>(packet.size()), 0);
#else
    const auto sent = send(socket_, packet.data(), packet.size(), MSG_NOSIGNAL);
#endif
    if (sent != static_cast<decltype(sent)>(packet.size()))
    {
        Disconnect();
        return false;
    }
    return true;
}

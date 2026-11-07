#include "TcpClient.h"

#include <array>
#include <cstdint>

#if defined(_WIN32)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <cerrno>
#endif

namespace {
void WriteU64BE(std::uint8_t* p, std::uint64_t value) {
    for (int i = 7; i >= 0; --i) { p[i] = static_cast<std::uint8_t>(value); value >>= 8; }
}
std::uint64_t ReadU64BE(const std::uint8_t* p) {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) value = (value << 8) | p[i];
    return value;
}
}

TcpClient::~TcpClient() { Disconnect(); }
bool TcpClient::Connect(const char* host, std::uint16_t port)
{
    Disconnect();
#if defined(_WIN32)
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
    winsockStarted_ = true;
#endif
    const auto candidate = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if defined(_WIN32)
    if (candidate == INVALID_SOCKET)
#else
    if (candidate < 0)
#endif
    { Disconnect(); return false; }
    socket_ = static_cast<Socket>(candidate);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &address.sin_addr) != 1 ||
        ::connect(static_cast<decltype(candidate)>(socket_),
                  reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
    { Disconnect(); return false; }
#if defined(_WIN32)
    u_long nonBlocking = 1;
    if (ioctlsocket(static_cast<SOCKET>(socket_), FIONBIO, &nonBlocking) != 0)
#else
    const int flags = fcntl(socket_, F_GETFL, 0);
    if (flags < 0 || fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0)
#endif
    { Disconnect(); return false; }
    sendBytes_ = 0; ackBytes_ = 0;
    return true;
}
void TcpClient::Disconnect()
{
    if (socket_ != InvalidSocket) {
#if defined(_WIN32)
        closesocket(static_cast<SOCKET>(socket_));
#else
        close(socket_);
#endif
        socket_ = InvalidSocket;
    }
#if defined(_WIN32)
    if (winsockStarted_) { WSACleanup(); winsockStarted_ = false; }
#endif
    sendBytes_ = 0; ackBytes_ = 0;
}
bool TcpClient::IsConnected() const { return socket_ != InvalidSocket; }

bool TcpClient::SendFrame(int controller, std::uint64_t frameNumber, const ControllerState& c)
{
    if (!IsConnected() || controller < 0 || controller > 3) return false;
    if (sendBytes_ == 0) {
        std::uint16_t mask = 0;
        if (c.dRight) mask |= 1u << 0;
        if (c.dLeft) mask |= 1u << 1;
        if (c.dDown) mask |= 1u << 2;
        if (c.dUp) mask |= 1u << 3;
        if (c.start) mask |= 1u << 4;
        if (c.z) mask |= 1u << 5;
        if (c.b) mask |= 1u << 6;
        if (c.a) mask |= 1u << 7;
        if (c.cRight) mask |= 1u << 8;
        if (c.cLeft) mask |= 1u << 9;
        if (c.cDown) mask |= 1u << 10;
        if (c.cUp) mask |= 1u << 11;
        if (c.r) mask |= 1u << 12;
        if (c.l) mask |= 1u << 13;
        sendBuffer_ = {'O','T','6','4',2,static_cast<std::uint8_t>(controller),
            0,0,0,0,0,0,0,0,
            static_cast<std::uint8_t>((mask >> 8) & 0xFF),
            static_cast<std::uint8_t>(mask & 0xFF),
            static_cast<std::uint8_t>(c.stickX), static_cast<std::uint8_t>(c.stickY)};
        WriteU64BE(sendBuffer_.data() + 6, frameNumber);
    }
    while (sendBytes_ < sendBuffer_.size()) {
#if defined(_WIN32)
        const int n = send(static_cast<SOCKET>(socket_),
            reinterpret_cast<const char*>(sendBuffer_.data() + sendBytes_),
            static_cast<int>(sendBuffer_.size() - sendBytes_), 0);
        if (n == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) return false;
            Disconnect(); return false;
        }
#else
        const auto n = send(socket_, sendBuffer_.data() + sendBytes_,
                            sendBuffer_.size() - sendBytes_, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return false;
            Disconnect(); return false;
        }
#endif
        if (n == 0) { Disconnect(); return false; }
        sendBytes_ += static_cast<std::size_t>(n);
    }
    sendBytes_ = 0;
    return true;
}

bool TcpClient::PollAcknowledgement(std::uint64_t& frameNumber)
{
    if (!IsConnected()) return false;
    while (ackBytes_ < ackBuffer_.size()) {
#if defined(_WIN32)
        const int n = recv(static_cast<SOCKET>(socket_),
            reinterpret_cast<char*>(ackBuffer_.data() + ackBytes_),
            static_cast<int>(ackBuffer_.size() - ackBytes_), 0);
        if (n == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) return false;
            Disconnect(); return false;
        }
#else
        const auto n = recv(socket_, ackBuffer_.data() + ackBytes_,
                            ackBuffer_.size() - ackBytes_, 0);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return false;
            Disconnect(); return false;
        }
#endif
        if (n == 0) { Disconnect(); return false; }
        ackBytes_ += static_cast<std::size_t>(n);
    }
    if (ackBuffer_[0] != 'A' || ackBuffer_[1] != 'C' ||
        ackBuffer_[2] != 'K' || ackBuffer_[3] != '2' || ackBuffer_[4] != 2 ||
        ackBuffer_[5] != 0) {
        Disconnect(); return false;
    }
    frameNumber = ReadU64BE(ackBuffer_.data() + 6);
    ackBytes_ = 0;
    return true;
}

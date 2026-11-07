#pragma once

#include "../tas/ControllerState.h"
#include <cstdint>

class TcpClient
{
public:
    TcpClient() = default;
    ~TcpClient();
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    bool Connect(const char* host = "127.0.0.1", std::uint16_t port = 6464);
    void Disconnect();
    bool IsConnected() const;
    bool SendController(int controller, const ControllerState& state);

private:
#if defined(_WIN32)
    using Socket = std::uintptr_t;
    static constexpr Socket InvalidSocket = static_cast<Socket>(~Socket{0});
#else
    using Socket = int;
    static constexpr Socket InvalidSocket = -1;
#endif
    Socket socket_ = InvalidSocket;
    bool winsockStarted_ = false;
};

#include "Plugin.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
using Socket = SOCKET;
constexpr Socket kInvalidSocket = INVALID_SOCKET;
static void CloseSocket(Socket s) { if (s != kInvalidSocket) closesocket(s); }
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <unistd.h>
using Socket = int;
constexpr Socket kInvalidSocket = -1;
static void CloseSocket(Socket s) { if (s != kInvalidSocket) close(s); }
#endif

namespace {
constexpr std::uint16_t kPort = 6464;
constexpr std::size_t kPacketSize = 10;
constexpr std::uint16_t kButtonMask = 0x3FFF;

struct ControllerState {
    std::atomic<std::uint16_t> buttons{0};
    std::atomic<std::int8_t> stickX{0};
    std::atomic<std::int8_t> stickY{0};
};

std::array<ControllerState, 4> gControllers;
std::atomic<bool> gRunning{false};
std::thread gServerThread;
std::atomic<Socket> gActiveClient{kInvalidSocket};
void* gDebugContext = nullptr;
void (*gDebugCallback)(void*, int, const char*) = nullptr;

void Log(int level, const char* message) {
    if (gDebugCallback) gDebugCallback(gDebugContext, level, message);
}

bool ReceiveExact(Socket client, std::uint8_t* data, std::size_t length) {
    std::size_t received = 0;
    while (received < length && gRunning.load()) {
#if defined(_WIN32)
        const int n = recv(client, reinterpret_cast<char*>(data + received),
                           static_cast<int>(length - received), 0);
#else
        const auto n = recv(client, data + received, length - received, 0);
#endif
        if (n <= 0) return false;
        received += static_cast<std::size_t>(n);
    }
    return received == length;
}

void ResetControllers() {
    for (auto& controller : gControllers) {
        controller.buttons.store(0);
        controller.stickX.store(0);
        controller.stickY.store(0);
    }
}

void HandleClient(Socket client) {
    gActiveClient.store(client);
    std::uint8_t packet[kPacketSize]{};
    while (gRunning.load() && ReceiveExact(client, packet, sizeof(packet))) {
        if (std::memcmp(packet, "OT64", 4) != 0 || packet[4] != 1 || packet[5] > 3)
            continue;
        const auto port = packet[5];
        const auto buttons = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(packet[6]) << 8) | packet[7]);
        gControllers[port].buttons.store(buttons & kButtonMask);
        gControllers[port].stickX.store(static_cast<std::int8_t>(packet[8]));
        gControllers[port].stickY.store(static_cast<std::int8_t>(packet[9]));
    }
    ResetControllers();
    gActiveClient.store(kInvalidSocket);
}

void ServerLoop() {
    Socket listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == kInvalidSocket) {
        Log(1, "OpenTAS64 input bridge: could not create TCP socket.");
        gRunning.store(false);
        return;
    }

    int reuse = 1;
#if defined(_WIN32)
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(kPort);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
        listen(listener, 1) != 0) {
        Log(1, "OpenTAS64 input bridge: port 6464 is unavailable.");
        CloseSocket(listener);
        gRunning.store(false);
        return;
    }

    Log(3, "OpenTAS64 input bridge listening on 127.0.0.1:6464.");
    while (gRunning.load()) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listener, &readSet);
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
#if defined(_WIN32)
        const int ready = select(0, &readSet, nullptr, nullptr, &timeout);
#else
        const int ready = select(listener + 1, &readSet, nullptr, nullptr, &timeout);
#endif
        if (!gRunning.load()) break;
        if (ready <= 0) continue;

        sockaddr_in peer{};
#if defined(_WIN32)
        int peerLength = sizeof(peer);
#else
        socklen_t peerLength = sizeof(peer);
#endif
        const Socket client = accept(listener, reinterpret_cast<sockaddr*>(&peer), &peerLength);
        if (client == kInvalidSocket) continue;
        Log(3, "OpenTAS64 editor connected.");
        HandleClient(client);
        CloseSocket(client);
        Log(3, "OpenTAS64 editor disconnected.");
    }

    CloseSocket(listener);
    ResetControllers();
}

void StopServer() {
    gRunning.store(false);
    const Socket client = gActiveClient.load();
    if (client != kInvalidSocket) {
#if defined(_WIN32)
        shutdown(client, SD_BOTH);
#else
        shutdown(client, SHUT_RDWR);
#endif
    }
    if (gServerThread.joinable()) gServerThread.join();
    ResetControllers();
}
}

m64p_error OT64_CALL PluginStartup(m64p_dynlib_handle, void* context,
                                    void (*debugCallback)(void*, int, const char*)) {
    if (gRunning.load()) return M64ERR_ALREADY_INIT;
#if defined(_WIN32)
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return M64ERR_SYSTEM_FAIL;
#endif
    if (gServerThread.joinable()) gServerThread.join();
    gDebugContext = context;
    gDebugCallback = debugCallback;
    gRunning.store(true);
    try {
        gServerThread = std::thread(ServerLoop);
    } catch (...) {
        gRunning.store(false);
#if defined(_WIN32)
        WSACleanup();
#endif
        return M64ERR_SYSTEM_FAIL;
    }
    return M64ERR_SUCCESS;
}

m64p_error OT64_CALL PluginShutdown() {
    StopServer();
#if defined(_WIN32)
    WSACleanup();
#endif
    gDebugCallback = nullptr;
    gDebugContext = nullptr;
    return M64ERR_SUCCESS;
}

m64p_error OT64_CALL PluginGetVersion(m64p_plugin_type* type, int* pluginVersion,
                                      int* apiVersion, const char** name, int* capabilities) {
    static const char pluginName[] = "OpenTAS64 Network Input";
    if (type) *type = M64PLUGIN_INPUT;
    if (pluginVersion) *pluginVersion = OT64_PLUGIN_VERSION;
    if (apiVersion) *apiVersion = OT64_INPUT_PLUGIN_API_VERSION;
    if (name) *name = pluginName;
    if (capabilities) *capabilities = 0;
    return M64ERR_SUCCESS;
}

void OT64_CALL InitiateControllers(OT64_CONTROL_INFO info) {
    if (!info.Controls) return;
    for (int i = 0; i < 4; ++i) {
        info.Controls[i].Present = 1;
        info.Controls[i].RawData = 0;
        info.Controls[i].Plugin = 1;
        info.Controls[i].Type = 0;
    }
}

void OT64_CALL GetKeys(int control, OT64_BUTTONS* keys) {
    if (!keys) return;
    keys->Value = 0;
    if (control < 0 || control >= static_cast<int>(gControllers.size())) return;
    const auto buttons = gControllers[control].buttons.load();
    const auto x = static_cast<std::uint8_t>(gControllers[control].stickX.load());
    const auto y = static_cast<std::uint8_t>(gControllers[control].stickY.load());
    keys->Value = static_cast<std::uint32_t>(buttons) |
                  (static_cast<std::uint32_t>(x) << 16) |
                  (static_cast<std::uint32_t>(y) << 24);
}

void OT64_CALL ControllerCommand(int, unsigned char*) {}
void OT64_CALL ReadController(int, unsigned char*) {}
int OT64_CALL RomOpen() { return 1; }
void OT64_CALL RomClosed() { ResetControllers(); }

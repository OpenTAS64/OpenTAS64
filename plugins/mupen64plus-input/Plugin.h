#pragma once

#include <cstdint>

#if defined(_WIN32)
#  define OT64_EXPORT extern "C" __declspec(dllexport)
#  define OT64_CALL __cdecl
#else
#  define OT64_EXPORT extern "C" __attribute__((visibility("default")))
#  define OT64_CALL
#endif

// Minimal ABI declarations for the Mupen64Plus v2 input-plugin interface.
// Kept local so OpenTAS64 does not copy or redistribute Mupen64Plus SDK headers.
using m64p_dynlib_handle = void*;
enum m64p_error { M64ERR_SUCCESS = 0, M64ERR_NOT_INIT = 1, M64ERR_ALREADY_INIT = 2,
    M64ERR_INCOMPATIBLE = 3, M64ERR_INPUT_ASSERT = 4, M64ERR_INPUT_INVALID = 5,
    M64ERR_INPUT_NOT_FOUND = 6, M64ERR_NO_MEMORY = 7, M64ERR_FILES = 8,
    M64ERR_INTERNAL = 9, M64ERR_INVALID_STATE = 10, M64ERR_PLUGIN_FAIL = 11,
    M64ERR_SYSTEM_FAIL = 12, M64ERR_UNSUPPORTED = 13, M64ERR_WRONG_TYPE = 14 };
enum m64p_plugin_type { M64PLUGIN_NULL = 0, M64PLUGIN_RSP = 1, M64PLUGIN_GFX = 2,
    M64PLUGIN_AUDIO = 3, M64PLUGIN_INPUT = 4, M64PLUGIN_CORE = 5 };

struct OT64_CONTROL { int Present; int RawData; int Plugin; int Type; };
struct OT64_CONTROL_INFO { OT64_CONTROL* Controls; };
union OT64_BUTTONS { std::uint32_t Value; };

constexpr int OT64_INPUT_PLUGIN_API_VERSION = 0x020100;
constexpr int OT64_PLUGIN_VERSION = 0x00010000;

OT64_EXPORT m64p_error OT64_CALL PluginStartup(m64p_dynlib_handle, void*, void (*)(void*, int, const char*));
OT64_EXPORT m64p_error OT64_CALL PluginShutdown();
OT64_EXPORT m64p_error OT64_CALL PluginGetVersion(m64p_plugin_type*, int*, int*, const char**, int*);
OT64_EXPORT void OT64_CALL InitiateControllers(OT64_CONTROL_INFO);
OT64_EXPORT void OT64_CALL GetKeys(int, OT64_BUTTONS*);
OT64_EXPORT void OT64_CALL ControllerCommand(int, unsigned char*);
OT64_EXPORT void OT64_CALL ReadController(int, unsigned char*);
OT64_EXPORT int OT64_CALL RomOpen();
OT64_EXPORT void OT64_CALL RomClosed();

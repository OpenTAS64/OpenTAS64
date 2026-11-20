#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

namespace OpenTAS64Icon {

// Compact BMP derived from the OpenTAS64 controller mark. Embedded so the
// icon is available regardless of the application's working directory.
inline SDL_Surface* Create()
{
    static const char encoded[] =
        "Qk02AwAAAAAAADYAAAAoAAAAEAAAABAAAAABABgAAAAAAAADAADEDgAAxA4AAAAAAAAAAAAAFBQUFBQUFhYWFhYWFhYWFhYWFxYWExMTIiIiFRUVFRYWFhYWFhYWFBQUFBQUFBQUFBQUFBQUFhYWFhYWFhYWFxcXERERLS4uT09PNTY2ERERFxcXFxcXFBQUFBQUFBQUFBQUFBQUFhYWFBQUExMTFRUVFBUVSEhIFBUVRUZGHBwcFBQUExMTFBQUFBQUFBQUFBQUFBQUERERNjY2TExNExMUISIiPj4/BwcHNzc3LS4tDw8PPz8/FBQUFBQUFBQUFBQUFBQUGBgYP0BANjc3Ly8vIiIiQkJCKisrPDw8MC8vIiEhQ0REFBQUFBQUFBQUFBQUFBQUHBwcQUFCFBUVRERERUVFT09PiIiKVlZWQkREP0RIIiIjFBQUFBQUFBQUFBQUFBQUGhoaREREEBARLC0tJCQkKysrX19YMjQ0LCEiTzUnDg0OFBQUFBQUFBQUFBQUFBQUFxcXPz8/MjIyS0xLJSUlEBAPHBpdEhYfL0QUREc9FTE8FBQUFBQUFBQUFBQUFBQUEhISQUFBNjY2RkZGKSkpFBUUGRg0GRgbGi0eEFVpFlNoFBQUFBQUFBQUFBQUFBQUERESJygoR0dHLCwsFRUVDw8PEBAKDg8PFA4RKDQ4PEdKFBQUFBQUFBQUFBQUFBQUFhcXERISIiIjOjo6SkpLNzc3NDQ1NTU2SUpKPz08LCoqFBQUFBQUFBQUFBQUFBQUFhYWFxcXEhMTDg4OGRoaLS0tMDAxLy8vHh4eDg8PEBERFBQUFBQUFBQUFBQUFBQUFRYWFRYWFRYWFxcXFBUVEBEREBAQDxAQExMUFxcXFhYXFBQUFBQUFBQUFBQUFBQUFhYWFRYWFRUWFRUWFhYWFxcXFxcXFhcXFhYWFRYWFRYWFBQUFBQUFBQUFBQUFBQUFhYWFhYWFhYWFhYWFhYWFhYWFRYWFhYWFhYWFhYWFhYWFBQUFBQUFBQUFBQUFBQUFhYWFhYWFhYWFhYWFhYWFRYWFRYWFRYWFhYWFhYWFhYWFBQUFBQUFBQU";
    static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<std::uint8_t> bytes;
    unsigned int buffer = 0;
    int bits = 0;
    for (const char* p = encoded; *p; ++p) {
        if (*p == '=') break;
        const char* found = alphabet;
        while (*found && *found != *p) ++found;
        if (!*found) continue;
        buffer = (buffer << 6) | static_cast<unsigned int>(found - alphabet);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            bytes.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xff));
        }
    }
    if (bytes.empty()) return nullptr;
    SDL_RWops* stream = SDL_RWFromConstMem(bytes.data(), static_cast<int>(bytes.size()));
    return stream ? SDL_LoadBMP_RW(stream, 1) : nullptr;
}

} // namespace OpenTAS64Icon

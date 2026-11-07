# 🎮 OpenTAS64

> **A modern, open-source TAS toolkit for Nintendo 64 emulators.**

OpenTAS64 is a standalone application for creating, editing, and analyzing Nintendo 64 Tool-Assisted Speedruns (TAS). It is designed to connect to existing emulators rather than replace them.

> **🚧 Status:** Early development — offline editing, native project files, and the first Mupen64Plus input bridge are implemented.

## Implemented

- SDL2 desktop application with Dear ImGui docking interface
- Emulator-independent controller and input-frame data model
- Offline editor for N64 buttons and analog stick X/Y
- Frame timeline with frame selection, insertion, and deletion
- Native `.otas` project save/load with versioned format
- Mupen64Plus 2.x network input plugin
- Local TCP connection between the editor and plugin
- Live selected-frame controller state forwarding to controller port 1

The connection currently forwards the selected frame's state as a live controller state. Frame-accurate playback, emulator pause/reset/frame advance, movie conversion, and acknowledgements are not implemented yet.

## Mupen64Plus connection

1. Build the `OpenTAS64MupenInput` target.
2. Select the resulting shared library as the Mupen64Plus **Input Plugin**.
3. Start Mupen64Plus with a ROM.
4. Open OpenTAS64 and press **Connect to plugin** in the Connection panel.

The plugin listens only on `127.0.0.1:6464`. See [plugins/mupen64plus-input/README.md](plugins/mupen64plus-input/README.md) and [docs/CONNECTION_PROTOCOL.md](docs/CONNECTION_PROTOCOL.md).

## Feature status

| Feature | Status |
|---|:---:|
| Offline Input Editor | ✅ Implemented |
| Frame Timeline | ✅ Implemented |
| Native Project Save / Load | ✅ Implemented |
| Mupen64Plus Input Plugin | 🧪 Initial bridge |
| Live Controller Forwarding | 🧪 Initial bridge |
| Piano Roll | ⏳ Planned |
| Frame-Accurate TAS Playback | ⏳ Planned |
| Emulator Frame Advance / Pause | ⏳ Planned |
| Savestate Manager | ⏳ Planned |
| RAM Watch / Search | ⏳ Planned |
| Lua Scripting | ⏳ Planned |
| Movie Import / Export | ⏳ Planned |

## Build requirements

- C++17 compiler
- CMake 3.16+
- SDL2 development package
- Git submodules initialized (Dear ImGui)

```bash
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Architecture and documentation

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [Native project format](docs/PROJECT_FORMAT.md)
- [Connection protocol](docs/CONNECTION_PROTOCOL.md)
- [Mupen64Plus plugin setup](plugins/mupen64plus-input/README.md)

## License

MIT — see [LICENSE](LICENSE).

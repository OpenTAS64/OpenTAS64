# 🎮 OpenTAS64

> **An open-source TAS editor and emulator input bridge for Nintendo 64.**

OpenTAS64 is a standalone desktop application for creating, editing, and playing back Nintendo 64 Tool-Assisted Speedrun (TAS) input projects. It connects to existing emulators instead of replacing them.

> **🚧 Status:** Early development. The desktop editor, native project format, vertical Piano Roll, Mupen64Plus input plugin, and input-poll-based single-step/lockstep controls are implemented. The initial timeline contains 120 editable frames. The new pause gate and multi-frame playback are awaiting runtime verification.

## Implemented

- C++17 desktop application using SDL2 and Dear ImGui
- Emulator-independent controller and input-frame data model
- Offline editor for N64 buttons and analog stick X/Y
- 120-frame initial timeline with frame selection, insertion, and deletion
- Vertical Piano Roll with button columns and one row per frame
- Direct editing of button states and analog stick values in the Piano Roll
- Native `.otas` project save/load with versioned format
- Mupen64Plus 2.x network input plugin
- Local TCP connection between the editor and plugin
- Protocol v2 lockstep input delivery: one frame in flight at a time
- Input-poll acknowledgements from the Mupen64Plus `GetKeys` callback
- Single-step control that sends one frame and waits for its input-poll acknowledgement
- Emulator pause gate at controller-0 input polls
- Safe pause handling that drains the in-flight frame before pausing

## Synchronization status

The plugin acknowledges a frame after Mupen64Plus calls `GetKeys` and consumes the queued controller state. OpenTAS64 waits for that acknowledgement before advancing to the next input frame.

This is **not** a rendered-frame acknowledgement. It does not prove that the corresponding video frame has finished rendering or that the emulator's internal frame counter matches the TAS frame number. Therefore, Step Frame currently means **one acknowledged input poll**, not a guaranteed completed video frame. Exact frame synchronization requires a cooperating emulator-core or video-plugin callback.

The connection and input delivery have been implemented, but real-game input acceptance and timing still require manual verification.

## Mupen64Plus connection

1. Build the `OpenTAS64MupenInput` target.
2. Select the resulting shared library as the Mupen64Plus **Input Plugin**.
3. Start Mupen64Plus with a ROM.
4. Open OpenTAS64 and press **Connect to plugin** in the Connection panel.

The plugin listens only on `127.0.0.1:6464`. See [plugin setup](plugins/mupen64plus-input/README.md) and the [connection protocol](docs/CONNECTION_PROTOCOL.md).

## Feature status

| Feature | Status |
|---|:---:|
| Offline Input Editor | ✅ Implemented |
| Frame Timeline | ✅ Implemented |
| Vertical Piano Roll | ✅ Implemented |
| Native Project Save / Load | ✅ Implemented |
| Mupen64Plus Input Plugin | 🧪 Connects; game input still needs verification |
| Lockstep Input Delivery | 🧪 Input-poll acknowledged |
| Step Frame | 🧪 One input poll per step; rendered-frame sync not guaranteed |
| Rendered-frame synchronization | ⏳ Needs emulator-side frame-complete hook |
| Emulator Pause between controller-0 input polls | 🧪 Implemented; needs runtime verification |
| Emulator Reset / Native Frame Advance | ⏳ Planned |
| Savestate Manager | ⏳ Planned |
| RAM Watch / Search | ⏳ Planned |
| Lua Scripting | ⏳ Planned |
| Movie Import / Export | ⏳ Planned |

## Cross-platform installer

Run the Python bootstrapper from the repository root. It detects the operating system, asks for confirmation, installs supported dependencies, initializes submodules, builds the application and input plugin, and runs tests.

```bash
python3 install.py
```

On Windows, use `py install.py`.

See [Installation Guide](docs/INSTALLATION.md) for supported platforms and requirements.

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

- [Architecture](ARCHITECTURE.md)
- [Native project format](docs/PROJECT_FORMAT.md)
- [Connection protocol](docs/CONNECTION_PROTOCOL.md)
- [Mupen64Plus plugin setup](plugins/mupen64plus-input/README.md)
- [Runtime testing checklist](docs/TESTING.md)

## License

MIT — see [LICENSE](LICENSE).

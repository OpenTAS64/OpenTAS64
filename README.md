# 🎮 OpenTAS64

> **A modern, open-source TAS toolkit for Nintendo 64 emulators.**

OpenTAS64 is a standalone application for creating, editing, and analyzing Nintendo 64 Tool-Assisted Speedruns (TAS). It is designed to connect to existing emulators rather than replace them.

> **🚧 Status:** Early development — offline project editing and native project files are implemented.

## Implemented

- SDL2 desktop application with Dear ImGui docking interface
- Emulator-independent controller and input-frame data model
- Offline editor for N64 buttons and analog stick X/Y
- Frame timeline with frame selection, insertion, and deletion
- Native `.otas` project save/load
- Versioned project format with backward compatibility for version 1 files
- Project file validation and preservation of the current project on failed loads
- CTest unit tests for project editing and serialization

Projects can be edited and saved without an emulator. Emulator communication and movie conversion are not implemented yet; the connection panel remains unavailable until an adapter exists.

## Saving and loading projects

Open the **Project** panel, enter a path ending in `.otas`, and use **Save** or **Load**. The File menu also provides New, Save, and Load actions. Press **Ctrl+S** to save the current project.

The native format is documented in [docs/PROJECT_FORMAT.md](docs/PROJECT_FORMAT.md).

## Feature status

| Feature | Status |
|---|:---:|
| Offline Input Editor | ✅ Implemented |
| Frame Timeline | ✅ Implemented |
| Native Project Save / Load | ✅ Implemented |
| Versioned Project Format | ✅ Implemented |
| Piano Roll | ⏳ Planned |
| Frame Advance | ⏳ Planned |
| Emulator Connection | ⏳ Planned |
| Savestate Manager | ⏳ Planned |
| RAM Watch / Search | ⏳ Planned |
| Lua Scripting | ⏳ Planned |
| Movie Import / Export | ⏳ Planned |
| Rerecord Counter | ⏳ Planned |

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

## Emulator support

Mupen64Plus is the first planned integration. No emulator is currently connected.

## Roadmap

- **0.3:** Piano-roll view and movie playback controls
- **0.4:** RAM watch, RAM search, and hex viewer
- **0.5:** Lua scripting and overlay support
- **1.0:** Stable release, emulator integrations, documentation, and plugin API

See [ARCHITECTURE.md](ARCHITECTURE.md) for module boundaries and [docs/PROJECT_FORMAT.md](docs/PROJECT_FORMAT.md) for the native file format.

## Contributing

Issues and pull requests are welcome.

## License

MIT — see [LICENSE](LICENSE).

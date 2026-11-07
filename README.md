# 🎮 OpenTAS64

> **A modern, open-source TAS toolkit for Nintendo 64 emulators.**

OpenTAS64 is a standalone application for creating, editing, and analyzing Nintendo 64 Tool-Assisted Speedruns (TAS). It is designed to connect to existing emulators rather than replace them.

> **🚧 Status:** Early development — the offline TAS input editor is the first functional application feature.

## Implemented

- SDL2 desktop application with Dear ImGui docking interface
- Emulator-independent controller and input-frame data model
- Offline editor for N64 buttons and analog stick X/Y
- Frame timeline with frame selection, insertion, and deletion
- New-project reset

Projects currently exist in memory only. Saving/loading, movie conversion, and emulator communication are not implemented. The connection panel is intentionally unavailable until an emulator adapter exists.

## Feature status

| Feature | Status |
|---|:---:|
| Offline Input Editor | ✅ Implemented |
| Frame Timeline | ✅ Implemented |
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
```

## Emulator support

Mupen64Plus is the first planned integration. No emulator is currently connected.

## Roadmap

- **0.2:** Offline TAS data model, input editor, and timeline
- **0.3:** Piano-roll view and movie playback controls
- **0.4:** RAM watch, RAM search, and hex viewer
- **0.5:** Lua scripting and overlay support
- **1.0:** Stable release, project/movie formats, documentation, and plugin API

See [ARCHITECTURE.md](ARCHITECTURE.md) for module boundaries and the planned integration design.

## Contributing

Issues and pull requests are welcome.

## License

MIT — see [LICENSE](LICENSE).

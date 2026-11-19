# 🎮 OpenTAS64

> **An open-source TAS editor and emulator input bridge for Nintendo 64.**

OpenTAS64 is a standalone desktop application for creating, editing, and playing back Nintendo 64 Tool-Assisted Speedrun (TAS) input projects. It connects directly to Mupen64Plus through a dedicated input plugin; the Mupen64Plus Launcher is not part of the connection.

**[🌐 Project website](https://opentas64.github.io/OpenTAS64/)**

> **🚧 Status:** Early development. The desktop editor, native project format, vertical Piano Roll, Mupen64Plus input plugin, core-frame-boundary acknowledgements, emulator restart command, and replay-based rewind are implemented. Runtime behavior still needs verification with the target Mupen64Plus build.

## Implemented

- C++17 desktop application using SDL2 and Dear ImGui
- Emulator-independent controller and input-frame data model
- Offline editor for N64 buttons and analog stick X/Y
- 120-frame initial timeline with frame selection, insertion, and deletion
- Vertical Piano Roll with button columns and one row per frame
- Direct editing of button states and analog stick values in the Piano Roll
- Native .otas project save/load with versioned format
- Mupen64Plus 2.x network input plugin
- Direct local TCP connection between the editor and plugin
- Lockstep input delivery: one frame in flight at a time
- Input states latched across repeated `GetKeys` calls within an emulated frame
- Frame acknowledgements from Mupen64Plus' `M64CMD_SET_FRAME_CALLBACK` core callback
- Single-step control and multi-frame playback
- Pause handling that drains the in-flight input before pausing
- **Restart Emulator:** requests a Mupen64Plus reset and returns the editor's selected frame to the beginning
- **Rewind to selected:** resets the emulator and replays the saved TAS inputs from frame zero to the selected timeline position
- Rewind/restart keep the TAS project's recorded input data intact
- In-place updater that refreshes the existing checkout and rebuilds without reinstalling

## Synchronization and rewind notes

The plugin latches the queued controller state when Mupen64Plus calls `GetKeys`. It keeps that state active for repeated reads during the emulated frame. The core's `M64CMD_SET_FRAME_CALLBACK` callback then acknowledges the frame at the core's emulated frame boundary, and OpenTAS64 waits for that acknowledgement before sending the next input.

This is a **core frame-boundary acknowledgement**, not a guarantee that the host display has presented the corresponding image. It also does not guarantee that the emulator's internal frame counter matches the TAS frame number. Exact host-visible presentation timing would require a suitable graphics/video callback.

Rewind currently uses **reset and replay**, not per-frame savestates. The emulator is reset and the recorded TAS inputs are replayed from the beginning until the selected position. Longer rewinds therefore take longer. The project data is preserved.

The reset command is issued through the Mupen64Plus Core API from the plugin's bridge thread. Runtime behavior can depend on the Mupen64Plus build and its thread-safety guarantees. Verify reset and replay with the exact emulator build you use before relying on them for a recording.

## Mupen64Plus connection

1. Build the OpenTAS64MupenInput target.
2. Select the resulting shared library as the Mupen64Plus Input Plugin.
3. Start Mupen64Plus with a ROM.
4. Open OpenTAS64 and press **Connect to plugin** in the Connection panel.

The plugin listens only on `127.0.0.1:6464`. The editor communicates directly with the plugin; no launcher integration is required. See [plugin setup](plugins/mupen64plus-input/README.md) and the [connection protocol](docs/CONNECTION_PROTOCOL.md).

## Feature status

| Feature | Status |
|---|:---:|
| Offline Input Editor | ✅ Implemented |
| Frame Timeline and Vertical Piano Roll | ✅ Implemented |
| Native Project Save / Load | ✅ Implemented |
| Mupen64Plus Input Plugin | 🧪 Connects; game input needs verification |
| Lockstep Input Delivery and Step Frame | 🧪 Input-poll acknowledged |
| Pause and multi-frame playback | 🧪 Needs runtime verification |
| Emulator Restart | 🧪 Implemented; verify with target core |
| Rewind to selected (reset + replay) | 🧪 Implemented; verify replay timing |
| Core frame-boundary synchronization | 🧪 Implemented; needs runtime verification |
| Host display presentation synchronization | ⏳ Not implemented |
| Savestate Manager | ⏳ Planned |
| RAM Watch / Search | ⏳ Planned |
| Lua Scripting | ⏳ Planned |
| Movie Import / Export | ⏳ Planned |

## Cross-platform installer

Run the Python bootstrapper from the repository root. It detects the operating system, asks for confirmation, installs supported dependencies, initializes submodules, builds the application and input plugin, and runs tests.

~~~bash
python3 install.py
~~~

On Windows, use `py install.py`.

See [Installation Guide](docs/INSTALLATION.md) for supported platforms and requirements.

## In-place updates

If you already have an OpenTAS64 source checkout, update it without running the installer again:

~~~bash
python3 update.py
~~~

On Windows:

~~~powershell
py update.py
~~~

The updater fast-forwards the existing Git checkout, refreshes its submodules, rebuilds the application and Mupen64Plus input plugin, and runs the tests. It does not reinstall system packages or recreate the installation. It preserves the local `imgui.ini` window layout and leaves untracked `.otas` projects and other user files untouched. Close OpenTAS64 before updating, especially on Windows.

The updater expects the existing Git checkout and build prerequisites from the initial setup. If tracked source files have local modifications, it stops instead of overwriting them.

## Build requirements

- C++17 compiler
- CMake 3.16+
- SDL2 development package
- Git submodules initialized (Dear ImGui)

~~~
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure
~~~

## Architecture and documentation

- [Architecture](ARCHITECTURE.md)
- [Native project format](docs/PROJECT_FORMAT.md)
- [Connection protocol](docs/CONNECTION_PROTOCOL.md)
- [Mupen64Plus plugin setup](plugins/mupen64plus-input/README.md)
- [Runtime testing checklist](docs/TESTING.md)

## Website deployment

The static project website lives in `website/` and is deployed by the GitHub Actions workflow in `.github/workflows/pages.yml`.

To enable it once, open **Settings → Pages** in the repository and select **GitHub Actions** as the build and deployment source. After that, pushes to `main` that change `website/` deploy the site automatically.

## License

MIT — see LICENSE.

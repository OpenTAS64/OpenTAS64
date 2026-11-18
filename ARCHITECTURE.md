# OpenTAS64 Architecture

OpenTAS64 separates the emulator-independent TAS project model, persistence, desktop application, and emulator integration.

## Repository modules

- `src/tas/`: controller state, TAS frames, timeline operations, and native project serialization.
- `src/core/`: SDL2 application lifecycle, project controls, playback state, and Dear ImGui application UI.
- `src/network/`: desktop-side TCP client for the local Mupen64Plus plugin bridge, including frame and reset commands.
- `plugins/mupen64plus-input/`: Mupen64Plus 2.x input plugin and loopback TCP server.
- `src/emulator/`: reserved for future emulator-control abstractions; it is not currently an implemented backend.
- `tests/`: automated tests for the TAS project model and persistence.
- `third_party/`: vendored/submodule dependencies, including Dear ImGui.

## Connection design

The editor connects directly to the Mupen64Plus input plugin over TCP on `127.0.0.1:6464`. The Mupen64Plus Launcher is not involved.

The control protocol is version 3. It retains the existing version-2 frame packet and acknowledgement layout and adds the `RST2` reset command with a `RACK` response. Frame delivery is lockstep: only one input frame is in flight at a time.

The plugin receives packets on a worker thread and makes the queued controller state available through the emulator's normal `GetKeys` callback. Controller 0 waits until the editor supplies an input frame, consumes that state, and signals the worker thread to acknowledge the input poll. The editor waits for the acknowledgement before advancing to the next TAS input.

The plugin binds exclusively to `127.0.0.1`; it is not exposed on external network interfaces. On disconnect, controller state is reset to neutral and a waiting input callback is released.

## Editor and playback

The timeline and vertical Piano Roll edit the same emulator-independent frame data. The Piano Roll displays controller buttons as columns and input frames as rows. Each frame also stores analog stick X/Y values.

A new project starts with 120 editable frames. Lockstep playback sends one input frame at a time and waits for its input-poll acknowledgement. **Step Frame** uses the same mechanism for a single input poll. Pause stops scheduling new inputs and allows an in-flight acknowledgement to finish.

The acknowledgement confirms that the input plugin's `GetKeys` callback consumed the input. It does not confirm that the corresponding video frame finished rendering or that the emulator's internal frame counter matches the TAS frame number.

## Restart and rewind

**Restart Emulator** sends the `RST2` control packet. The plugin resolves Mupen64Plus's `CoreDoCommand` function and requests `M64CMD_RESET`. After a successful response, the editor returns its selected timeline position to frame zero. The TAS input data remains unchanged.

**Rewind to selected** is currently implemented as reset and replay, not as a savestate restore. The editor requests a reset and then replays the stored TAS inputs from frame zero until the selected timeline position. This preserves the project data but makes longer rewinds take longer.

The reset request is issued from the plugin's bridge thread. Whether this Core API call is safe in that context depends on the Mupen64Plus build and its threading behavior. Restart and replay must be verified with the exact emulator build in use. A successful input-poll acknowledgement is not a rendered-frame synchronization guarantee.

## Design rules

1. TAS data must not depend on SDL, ImGui, or a specific emulator.
2. The UI edits the project model through its public interface.
3. Network I/O runs on the plugin worker thread; the controller callback waits for queued input and performs no socket I/O.
4. Emulator-specific behavior belongs behind the plugin/integration boundary.
5. Frame indices are zero-based internally.
6. Every project contains at least one editable frame.
7. Never describe an input-poll acknowledgement as a rendered-frame acknowledgement.
8. Restart and rewind must not clear or rewrite the recorded TAS input data.

## Current limitations

- There is no emulator-side frame-complete callback, so exact rendered-frame synchronization is not implemented.
- Rewind replays inputs from the beginning; per-frame savestates are not implemented.
- Core reset is requested from the bridge thread and requires runtime verification against the target Mupen64Plus build.
- Actual in-game input acceptance, reset behavior, replay timing, pause/resume, and repeated stepping require manual testing.

## Next steps

- Verify controller input delivery, playback, pause/resume, repeated Step Frame, restart, and replay-based rewind in a running game.
- Investigate a supported emulator-side frame-complete callback for stronger synchronization.
- Investigate safe core-level control and savestate integration.
- Expand movie import/export and memory inspection features.

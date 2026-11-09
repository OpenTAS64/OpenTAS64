# OpenTAS64 Architecture

OpenTAS64 separates TAS data, persistence, the desktop UI, and emulator integrations.

## Modules

- `src/tas/`: emulator-independent controller states, input frames, timeline operations, and native project serialization.
- `src/core/`: SDL2 application lifecycle, project controls, timeline/Piano Roll presentation, playback state, and Dear ImGui UI.
- `src/network/`: desktop-side TCP client for the local plugin bridge, including frame delivery and acknowledgement handling.
- `plugins/mupen64plus-input/`: Mupen64Plus 2.x input plugin and loopback TCP server.
- `src/emulator/`: reserved for future emulator control APIs beyond controller input.
- `tests/`: standalone tests for the TAS project model and persistence layer.

## Connection design

The editor sends versioned, frame-indexed controller packets over loopback TCP. Protocol v2 permits only one frame in flight. The Mupen64Plus plugin receives packets on a worker thread and makes the queued state available to the emulator's normal `GetKeys` callback. The callback waits for the next controller-0 frame, latches it, and acknowledges that input poll. With no queued frame, the emulator remains blocked at that poll. The editor waits for the acknowledgement before sending the next frame.

The plugin binds exclusively to `127.0.0.1`; it is not exposed to other machines on the network. A disconnect resets controller state to neutral.

## Editor and playback

The timeline and vertical Piano Roll edit the same emulator-independent frame data. The Piano Roll displays controller buttons as columns and input frames as rows, with analog stick X/Y values available for each frame.

The timeline starts with 120 editable frames. Lockstep playback sends one input frame at a time and waits for its input-poll acknowledgement. The plugin's controller-0 callback waits at the next poll until another frame is supplied. The UI pause request stops scheduling further frames and lets any in-flight acknowledgement finish; the emulator then waits at the next controller-0 poll. This behavior still requires runtime verification. Step Frame uses the same mechanism for a single frame. It is therefore a single **input-poll step**, not yet a guarantee that the corresponding rendered video frame has completed.

## Design rules

1. TAS data must not depend on SDL, ImGui, or a specific emulator.
2. The UI edits the model through its public interface.
3. Network I/O runs on the plugin worker thread; the controller-0 callback may wait on a condition variable for the next queued input, but performs no socket I/O.
4. Emulator-specific behavior belongs behind an adapter/plugin boundary.
5. Frame indices are zero-based internally.
6. Every project contains at least one editable frame.
7. An input-poll acknowledgement must not be described as a rendered-frame acknowledgement.

## Current synchronization limitation

The standard Mupen64Plus input-plugin callback confirms when input is polled, but does not provide a reliable post-render frame-complete event. OpenTAS64 cannot currently prove that the emulator's internal frame counter matches the TAS frame number. Deterministic rendered-frame synchronization will require a cooperating core/video-plugin hook or another supported emulator-side frame callback.

## Next steps

- Verify controller input delivery, multi-frame playback, pause/resume, and repeated Step Frame behavior in a running game.
- Investigate a supported emulator-side frame-complete callback for stronger synchronization.
- Investigate core-level pause/reset and savestate integration for control beyond the controller-0 input-poll gate.
- Expand movie import/export and memory inspection features.

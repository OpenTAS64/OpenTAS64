# OpenTAS64 Architecture

OpenTAS64 separates TAS data, persistence, the desktop UI, and emulator integrations.

## Modules

- `src/tas/`: emulator-independent controller states, input frames, timeline operations, and native project serialization.
- `src/core/`: SDL2 application lifecycle, project controls, and Dear ImGui presentation.
- `src/network/`: desktop-side TCP client for the local plugin bridge.
- `plugins/mupen64plus-input/`: Mupen64Plus 2.x input plugin and loopback TCP server.
- `src/emulator/`: reserved for future emulator control APIs beyond controller input.
- `tests/`: standalone tests for the TAS project model and persistence layer.

## Connection design

The editor sends a compact, versioned controller-state packet over loopback TCP.
The Mupen64Plus plugin receives packets on a worker thread and publishes the
latest state through atomics. The emulator's normal `GetKeys` callback reads
that state without waiting on network I/O. A disconnect resets all controls.

The plugin binds exclusively to `127.0.0.1`; it is not exposed to other
machines on the network.

## Design rules

1. TAS data must not depend on SDL, ImGui, or a specific emulator.
2. The UI edits the model through its public interface.
3. Network I/O must never block the emulator's controller polling callback.
4. Emulator-specific behavior belongs behind an adapter/plugin boundary.
5. Frame indices are zero-based internally.
6. Every project contains at least one editable frame.

## Current connection limitation

The initial bridge forwards the selected frame as a live input state. It does
not provide frame-indexed delivery, frame acknowledgements, pause/reset, or
frame advance. These are required before claiming deterministic TAS playback.

## Next steps

- Add a frame-indexed protocol and explicit delivery acknowledgements.
- Implement playback scheduling and frame-advance integration.
- Add Mupen64Plus core control for pause, reset, and savestates.
- Add piano-roll view and movie import/export adapters.

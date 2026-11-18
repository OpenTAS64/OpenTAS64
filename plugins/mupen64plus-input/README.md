# OpenTAS64 Mupen64Plus Network Input Plugin

This is the emulator-side input bridge for Mupen64Plus 2.x. Mupen64Plus loads the shared library as its **Input** plugin. The plugin opens a TCP listener on `127.0.0.1:6464`; it does not listen on external network interfaces.

The desktop OpenTAS64 application connects directly to this local endpoint. The Mupen64Plus Launcher is not required and is not part of the connection.

## What it does

- Receives frame-indexed controller states from OpenTAS64.
- Supplies queued controller input through Mupen64Plus's `GetKeys` callback.
- Uses one-frame-at-a-time lockstep delivery and acknowledges a frame after the controller-0 input poll consumes it.
- Releases controller state to neutral when the editor disconnects.
- Handles the `RST2` reset control packet by requesting `M64CMD_RESET` through the Mupen64Plus Core API and replying with `RACK` plus a status byte.

The acknowledgement confirms an input poll, not completion of the corresponding rendered video frame.

## Build

Build from the repository root:

```bash
cmake -S . -B build
cmake --build build --config Release --target OpenTAS64MupenInput
```

The output is a platform shared library (DLL on Windows, shared object on Linux). Select it as the Mupen64Plus **Input Plugin**. The plugin is separate from the emulator executable and does not replace graphics, audio, or RSP.

## Connect

1. Start Mupen64Plus with this plugin selected and load a game.
2. Launch OpenTAS64.
3. Use **Connect to plugin** in the desktop application.
4. Confirm the connection status reports a successful local connection.

## Protocol and frame stepping

See [the protocol specification](../../docs/CONNECTION_PROTOCOL.md). The control protocol is version 3. The existing frame packet retains its version-2 packet layout; version 3 adds the reset control packet.

The plugin's controller-0 callback waits for the next queued input frame. It applies the state and signals the network worker, which sends the acknowledgement to the editor. The editor then advances to the next TAS frame.

**Step Frame** means one acknowledged input poll. It does not prove that a complete video frame has rendered. Exact video-frame synchronization requires a cooperating emulator core or video-plugin callback.

## Restart and rewind

**Restart Emulator** sends a reset command to the plugin. The plugin resolves `CoreDoCommand` from the loaded Mupen64Plus core and calls `M64CMD_RESET`. The editor resets its selected timeline position to frame zero while preserving the TAS project's recorded input data.

**Rewind to selected** performs a reset and replays the recorded inputs from the beginning until the selected timeline position. It does not currently use savestates, so rewind duration increases with the distance from the beginning.

The reset API call is made from the plugin's bridge thread. Its safety and behavior can vary with the Mupen64Plus build. Test restart and replay with the exact emulator version you intend to use. If `CoreDoCommand` is unavailable, reset controls are disabled by the plugin.

## Runtime verification

Actual in-game input acceptance, timing, reset behavior, and replay must be verified in a running game. See the [runtime testing checklist](../../docs/TESTING.md).

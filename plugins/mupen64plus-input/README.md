# OpenTAS64 Mupen64Plus Network Input Plugin

This is the emulator-side input bridge for Mupen64Plus 2.x. Mupen64Plus loads the shared library as its **Input** plugin. The plugin opens a TCP listener on `127.0.0.1:6464`; it does not listen on external network interfaces.

The desktop application connects to this local endpoint and sends controller states. The plugin's `GetKeys` callback returns the queued state to the emulator. If the editor disconnects, all controller states are released.

## Build

Build from the repository root:

```bash
cmake -S . -B build
cmake --build build --config Release --target OpenTAS64MupenInput
```

The output is a platform shared library (DLL on Windows, shared object on Linux). Select it as the Mupen64Plus **Input Plugin**. The plugin is separate from the emulator executable and does not replace graphics, audio, or RSP.

## Connection protocol and stepping

See [the protocol specification](../../docs/CONNECTION_PROTOCOL.md). Protocol v2 queues exactly one TAS input frame at a time. The plugin applies that frame when Mupen64Plus calls the input plugin's `GetKeys` callback, then sends an acknowledgement to the editor.

The editor uses this acknowledgement for lockstep playback and its **Step Frame** control. One acknowledgement means one input-poll callback consumed the input. It does **not** confirm that a complete video frame has rendered. Exact video-frame synchronization requires an emulator core or video-plugin callback. A TCP packet arriving does not itself guarantee a particular emulated frame.

Actual in-game input acceptance and timing should be verified with a running game.

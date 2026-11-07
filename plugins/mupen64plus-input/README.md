# OpenTAS64 Mupen64Plus Network Input Plugin

This is the emulator-side input bridge for Mupen64Plus 2.x. Mupen64Plus loads
the shared library as its **Input** plugin. The plugin opens a TCP listener on
`127.0.0.1:6464`; it does not listen on external network interfaces.

The editor connects to this local endpoint and sends controller states. The
plugin's `GetKeys` callback returns the most recently received state to the
emulator. If the editor disconnects, all controller states are released.

## Build

Build from the repository root:

```bash
cmake -S . -B build
cmake --build build --config Release --target OpenTAS64MupenInput
```

The output is a platform shared library (DLL on Windows, shared object on
Linux). Select it as the Mupen64Plus **Input Plugin**. The plugin is separate
from the normal emulator executable and does not replace the graphics, audio,
or RSP plugins.

## Connection protocol

The listener accepts one local editor connection at a time. Each message is a
fixed 10-byte packet:

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 4 | ASCII magic `OT64` |
| 4 | 1 | Protocol version (`1`) |
| 5 | 1 | Controller index (0–3) |
| 6 | 2 | Button mask, unsigned big-endian |
| 8 | 1 | Stick X, signed 8-bit |
| 9 | 1 | Stick Y, signed 8-bit |

Button mask uses the N64 order: Right, Left, Down, Up, Start, Z, B, A,
C-Right, C-Left, C-Down, C-Up, R, L.

This first bridge milestone transports live controller state. Frame scheduling,
acknowledgements, playback synchronization, and emulator frame control are
separate follow-up work; a TCP packet arriving does not itself guarantee a
particular emulated frame.

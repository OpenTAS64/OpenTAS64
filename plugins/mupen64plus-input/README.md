# OpenTAS64 Mupen64Plus Network Input Plugin

This is the emulator-side input bridge for Mupen64Plus 2.x. Mupen64Plus loads
the shared library as its **Input** plugin. The plugin opens a TCP listener on
`127.0.0.1:6464`; it does not listen on external network interfaces.

The desktop application connects to this local endpoint and sends controller
states. The plugin's `GetKeys` callback returns the latest state to the emulator.
If the editor disconnects, all controller states are released.

## Build

Build from the repository root:

```bash
cmake -S . -B build
cmake --build build --config Release --target OpenTAS64MupenInput
```

The output is a platform shared library (DLL on Windows, shared object on
Linux). Select it as the Mupen64Plus **Input Plugin**. The plugin is separate
from the emulator executable and does not replace graphics, audio, or RSP.

## Connection protocol

See [the protocol specification](../../docs/CONNECTION_PROTOCOL.md). This first
milestone sends the currently selected input state to controller port 1.
Frame scheduling, acknowledgements, playback synchronization, and emulator
frame control are not implemented yet. A TCP packet arriving does not itself
guarantee a particular emulated frame.

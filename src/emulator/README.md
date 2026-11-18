# Emulator Integration

This directory is currently reserved for future emulator-control abstractions.

The implemented Mupen64Plus integration currently lives in:

- `src/network/` — desktop-side TCP client.
- `plugins/mupen64plus-input/` — emulator-side Mupen64Plus input plugin.

Current capabilities include controller input delivery, input-poll acknowledgements, and a reset request. Rewind is performed by resetting and replaying TAS inputs from frame zero; native savestate control and rendered-frame synchronization are not implemented here yet.

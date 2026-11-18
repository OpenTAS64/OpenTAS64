# Input Data

Controller input data is currently defined in `src/tas/ControllerState.h`.

A controller state stores the N64 button states and signed analog stick X/Y coordinates. The project model is independent of SDL, Dear ImGui, and the Mupen64Plus plugin.

The current network bridge serializes these states into the frame packet described in [the connection protocol](../../docs/CONNECTION_PROTOCOL.md).

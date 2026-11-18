# Application Core

The application core coordinates the desktop program and its main loop.

Responsibilities:

- SDL2 window and renderer lifecycle.
- Dear ImGui frame lifecycle and application UI.
- TAS project ownership and editing commands.
- Connection state and playback coordination.
- Single input-poll stepping, pause handling, restart requests, and replay-based rewind orchestration.

The core does not implement the Mupen64Plus plugin itself. Emulator communication is handled by the TCP client in `src/network/`.

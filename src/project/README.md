# TAS Project Model

The current project model is implemented in `src/tas/`, rather than this placeholder directory.

- `ControllerState.h` defines controller button and analog stick state.
- `TasProject.h/.cpp` manage frames, frame selection, timeline edits, and native project persistence.
- The `.otas` format is documented in [PROJECT_FORMAT.md](../../docs/PROJECT_FORMAT.md).

Project data remains independent of the desktop UI and emulator integration. Restart and replay-based rewind operate on emulator state; they do not intentionally erase the project's recorded inputs.

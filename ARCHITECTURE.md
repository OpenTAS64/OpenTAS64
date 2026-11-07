# OpenTAS64 Architecture

OpenTAS64 separates TAS data from its desktop UI and future emulator integrations.

## Modules

- `src/tas/`: emulator-independent controller states, input frames, and project timeline.
- `src/core/`: SDL2 application lifecycle and Dear ImGui presentation.
- `src/emulator/`: reserved for emulator adapters; no emulator is connected yet.
- `plugins/`: reserved for optional integrations.

## Design rules

1. TAS data must not depend on SDL, ImGui, or a specific emulator.
2. The UI edits the model through its public interface.
3. Emulator-specific behavior belongs behind an adapter interface.
4. Frame indices are zero-based internally.
5. Every new project contains at least one editable frame.

## Next steps

- Versioned project/movie serialization.
- Emulator adapter API for connect, pause, reset, frame advance, input injection, and savestates.
- Piano-roll view and movie import/export adapters.

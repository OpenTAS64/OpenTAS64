# OpenTAS64 Architecture

OpenTAS64 separates TAS data, persistence, the desktop UI, and future emulator integrations.

## Modules

- `src/tas/`: emulator-independent controller states, input frames, timeline operations, and native project serialization.
- `src/core/`: SDL2 application lifecycle, file controls, and Dear ImGui presentation.
- `src/emulator/`: reserved for emulator adapters; no emulator is connected yet.
- `plugins/`: reserved for optional integrations.
- `tests/`: standalone tests for the TAS project model and persistence layer.

## Design rules

1. TAS data must not depend on SDL, ImGui, or a specific emulator.
2. The UI edits the model through its public interface.
3. File parsing validates into temporary data before replacing the active project.
4. Emulator-specific behavior belongs behind an adapter interface.
5. Frame indices are zero-based internally.
6. Every project contains at least one editable frame.

## Native project persistence

The `.otas` format is versioned and documented in `docs/PROJECT_FORMAT.md`.
Version 2 stores the selected timeline frame. The loader remains compatible with
version 1 projects. Parsing rejects malformed controller values and incomplete
files without mutating the active project.

## Next steps

- Piano-roll view and movie import/export adapters.
- Emulator adapter API for connect, pause, reset, frame advance, input injection,
  and savestate operations.
- Autosave and unsaved-change tracking.

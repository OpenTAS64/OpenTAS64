# User Interface

The desktop interface is currently rendered by Dear ImGui from the application core in `src/core/App.cpp`.

Implemented UI areas include:

- Project creation, loading, and saving.
- Controller button editor with clickable button controls.
- Two-dimensional draggable analog-stick pad with live X/Y readout and a center control; the previous independent axis sliders are not used.
- Frame timeline with previous/next, insert, and delete controls.
- Vertical Piano Roll input visualization and editing.
- Plugin connection status.
- Single input-poll step, lockstep playback, and pause controls.
- **Restart Emulator** and **Rewind to selected** controls.
- File-menu and keyboard Undo/Redo controls (`Ctrl+Z` / `Ctrl+Y`).
- Automatic project saving and restoration of the last-used project.

A dedicated docking layout, full menu system, and other planned UI areas are not described as implemented features yet.

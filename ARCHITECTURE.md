# OpenTAS64 Architecture

OpenTAS64 separates the emulator-independent TAS project model, persistence, desktop application, and emulator integration.

## Repository modules

- `src/tas/`: controller state, TAS frames, timeline operations, and native project serialization.
- `src/core/`: SDL2 application lifecycle, project controls, playback state, and Dear ImGui application UI.
- `src/network/`: desktop-side TCP client for the local Mupen64Plus plugin bridge, including frame and reset commands.
- `plugins/mupen64plus-input/`: Mupen64Plus 2.x input plugin and loopback TCP server.
- `src/emulator/`: reserved for future emulator-control abstractions; it is not currently an implemented backend.
- `tests/`: automated tests for the TAS project model and persistence.
- `third_party/`: vendored/submodule dependencies, including Dear ImGui.

## Connection design

The editor connects directly to the Mupen64Plus input plugin over TCP on `127.0.0.1:6464`. The Mupen64Plus Launcher is not involved.

The control protocol is version 3. It retains the existing version-2 frame packet and acknowledgement layout and adds the `RST2` reset command with a `RACK` response. Frame delivery is lockstep: only one input frame is in flight at a time.

The plugin receives packets on a worker thread and makes the queued controller state available through the emulator's normal `GetKeys` callback. Controller 0 latches each queued input state for the whole emulated frame, including repeated controller reads. The Mupen64Plus core frame callback signals the worker thread only after that emulated frame reaches the core frame boundary. The editor waits for this acknowledgement before advancing to the next TAS input.

The plugin binds exclusively to `127.0.0.1`; it is not exposed on external network interfaces. On disconnect, controller state is reset to neutral and a waiting input callback is released.

## Editor and playback

The timeline and vertical Piano Roll edit the same emulator-independent frame data. The Piano Roll displays controller buttons as columns and input frames as rows. Each frame also stores analog stick X/Y values.

A new project starts with 120 editable frames. The Timeline panel can append one or ten blank frames at a time and select the first newly added frame. The project format currently accepts up to 1,000,000 frames; the UI prevents extending beyond that limit. The Piano Roll uses row clipping so it only renders visible rows rather than processing the entire timeline every frame. Existing Insert and Delete operations remain available. Visible Undo and Redo buttons are provided in the Project panel as well as the Ctrl+Z / Ctrl+Y shortcuts. Lockstep playback sends one input frame at a time and waits for the core frame-boundary acknowledgement. **Step Frame** advances by one acknowledged emulated frame. Pause stops scheduling new inputs and allows an in-flight frame to reach its callback boundary.

The acknowledgement confirms that the Mupen64Plus core reached its frame callback after the input was latched. It does not guarantee that the host display presented the corresponding image or that the emulator's internal frame counter matches the TAS frame number.

## Input editor, project history, and persistence

The Input Editor can render the analog stick as a circular two-dimensional pad or as separate X/Y sliders. The selection is controlled from the **Options** menu. The visual pad lets the user drag its handle to set X/Y values; both modes clamp values to the controller's supported range and provide a center control. This is a UI preference only: the project model stores the same signed X/Y values per frame.

Project edits are tracked through snapshot-based Undo/Redo history, capped at 100 previous states. A new project or a successfully loaded project establishes a fresh history baseline. Undo and redo are disabled while playback, frame delivery, or reset is active. Keyboard shortcuts are Ctrl+Z and Ctrl+Y, with matching File-menu commands.

When enabled in the **Options** menu, autosave writes the project after 750 ms of inactivity when no input frame is in flight. Disabling autosave also disables the automatic shutdown save; manual project saving remains available. The autosave preference and stick-editor mode are stored in `opentas64_settings.ini`. The last used project path is stored in `last_project.txt` and used to restore the project at startup. The autosave uses the active project path; it does not create versioned backups.

## Restart and rewind

**Restart Emulator** sends the `RST2` control packet. The plugin resolves Mupen64Plus's `CoreDoCommand` function and requests `M64CMD_RESET`. After a successful response, the editor returns its selected timeline position to frame zero. The TAS input data remains unchanged.

**Rewind to selected** is currently implemented as reset and replay, not as a savestate restore. The editor requests a reset and then replays the stored TAS inputs from frame zero until the selected timeline position. This preserves the project data but makes longer rewinds take longer.

The reset request is issued from the plugin's bridge thread. Whether this Core API call is safe in that context depends on the Mupen64Plus build and its threading behavior. Restart and replay must be verified with the exact emulator build in use. A successful core frame-boundary acknowledgement is not a rendered-frame synchronization guarantee.

## Design rules

1. TAS data must not depend on SDL, ImGui, or a specific emulator.
2. The UI edits the project model through its public interface.
3. Network I/O runs on the plugin worker thread; the controller callback waits for queued input and performs no socket I/O.
4. Emulator-specific behavior belongs behind the plugin/integration boundary.
5. Frame indices are zero-based internally.
6. Every project contains at least one editable frame.
7. Never describe a core frame-boundary acknowledgement as a rendered-frame acknowledgement.
8. Restart and rewind must not clear or rewrite the recorded TAS input data.

## Current limitations

- The core callback confirms an emulated frame boundary, not host display presentation or completion of the graphics plugin's presentation path.
- Rewind replays inputs from the beginning; per-frame savestates are not implemented.
- Core reset is requested from the bridge thread and requires runtime verification against the target Mupen64Plus build.
- Actual in-game input acceptance, reset behavior, replay timing, pause/resume, and repeated stepping require manual testing.

## Next steps

- Verify controller input delivery, playback, pause/resume, repeated Step Frame, restart, and replay-based rewind in a running game.
- Investigate whether a graphics-plugin or video-presentation callback is needed for host-visible frame completion.
- Investigate safe core-level control and savestate integration.
- Expand movie import/export and memory inspection features.

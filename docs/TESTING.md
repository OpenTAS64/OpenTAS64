# OpenTAS64 Runtime Testing Checklist

This checklist separates automated build tests from manual checks that require a running Mupen64Plus instance and game. Frame acknowledgements are driven by Mupen64Plus' core frame callback. Do not treat them as proof that the host display has presented the corresponding image.

## Build and automated tests

For an existing checkout, use the in-place updater:

```bash
python3 update.py
```

On Windows:

```powershell
py update.py
```

The updater performs a fast-forward-only update, refreshes submodules, configures/builds the existing CMake tree, and runs CTest. It does not reinstall system dependencies. See [Installation and updating](INSTALLATION.md).

For a manual build:

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

If needed, build the input plugin target explicitly:

```bash
cmake --build build --config Release --target OpenTAS64MupenInput --parallel 2
```

## Manual emulator checks

1. Start Mupen64Plus with the OpenTAS64 input plugin selected and a game loaded.
2. Start OpenTAS64 and connect directly to the plugin. No launcher integration is used.
3. Confirm a new project starts with 120 editable timeline frames.
4. Select different timeline rows and confirm the Input Editor follows the selected frame.
5. Change button and stick values on multiple rows; confirm each row retains its own values when switching frames.
6. Insert and delete frames; confirm selection and neighboring input data remain consistent.
7. Use **Step Frame** repeatedly. Confirm each command is acknowledged at a Mupen64Plus core frame callback boundary. Check that repeated `GetKeys` calls within one emulated frame keep the same controller state active. Separately observe host-visible video presentation; the core callback does not guarantee that the display has presented the image.
8. Start playback and request **Pause**. Confirm the current in-flight input is acknowledged and no further TAS frame is sent.
9. Resume playback and confirm input delivery continues from the expected timeline position.
10. Use **Restart Emulator** while idle. Confirm Mupen64Plus resets, the editor selects frame 0, and the TAS input data is still present in the project.
11. Select a frame later in the timeline and use **Rewind to selected**. Confirm the emulator resets and recorded inputs replay from frame zero up to the selected position. Confirm the project data remains unchanged.
12. Repeat restart and rewind with the exact Mupen64Plus build intended for use. Record crashes, hangs, unexpected game state, or timing differences.
13. Disconnect while idle and confirm the plugin releases controller state to neutral. Reconnect and verify input delivery resumes.
14. Save and reload the project after restart/rewind; confirm the recorded frame inputs and selected-frame behavior are retained as expected.

## Important limitations

- The frame acknowledgement is emitted by `M64CMD_SET_FRAME_CALLBACK` after the input state has been latched and the core reaches its emulated frame boundary. It does not guarantee host display presentation.
- The pause gate is synchronized to the core frame callback and the next controller-0 input poll. It is not a general core-level pause command or a guarantee of host display presentation.
- Rewind currently uses reset plus input replay, not savestates.
- The plugin requests `M64CMD_RESET` through `CoreDoCommand` from its bridge thread. Runtime behavior depends on the target Mupen64Plus build and must be tested.
- Automated project tests do not replace these emulator runtime checks.

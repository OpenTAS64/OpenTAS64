# OpenTAS64 Runtime Testing Checklist

The pause gate and initial multi-frame timeline have been implemented, but have not yet been verified in a running game. Complete these checks before treating them as stable.

## Build

From the repository root:

```bash
git pull
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j2
ctest --test-dir build --output-on-failure
```

Build the input plugin if it is not included in the default build:

```bash
cmake --build build --config Release --target OpenTAS64MupenInput -j2
```

## Manual checks

1. Start Mupen64Plus with the OpenTAS64 input plugin selected, then start OpenTAS64 and connect.
2. Confirm the new project opens with 120 editable timeline frames.
3. Select different timeline rows and confirm the Input Editor follows the selected frame.
4. Change button and stick values on multiple rows; confirm each row retains its own values when switching between frames.
5. Insert and delete frames; confirm selection and neighboring input data remain consistent.
6. Start playback and request Pause. Confirm the current in-flight input is acknowledged and no further TAS frame is sent until playback resumes.
7. Resume playback and confirm frames continue from the expected timeline position.
8. Use Step Frame repeatedly and observe whether the game advances consistently by one input poll per step.
9. Disconnect the editor and confirm the plugin and emulator remain usable.

## Important limitations

- The acknowledgement confirms that Mupen64Plus consumed an input state in its `GetKeys` callback. It does not confirm that a rendered video frame completed.
- The pause gate waits at the next controller-0 input poll. It is not a core-level pause command and does not guarantee a video-frame boundary.
- Actual game behavior, pause/resume stability, and timing have not yet been verified. Record any unexpected behavior before making further synchronization changes.

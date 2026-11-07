# OpenTAS64 Project Format

OpenTAS64 uses the `.otas` extension for native project files. The format is a
versioned UTF-8 text format so it can be inspected and diffed during development.

## Version 2 layout

```text
OPENTAS64_PROJECT 2
"Project name"
<frame-count> <selected-frame>
<a> <b> <z> <start> <d-up> <d-down> <d-left> <d-right> <l> <r> <c-up> <c-down> <c-left> <c-right> <stick-x> <stick-y>
...
```

- Frame indices are zero-based.
- Button values are `0` (released) or `1` (pressed).
- Stick coordinates are signed integers from `-128` to `127`.
- Exactly one controller state is stored per frame.
- A project always contains at least one frame.
- The selected frame is stored when saving and restored when loading.

## Compatibility and validation

The loader accepts version 1 files created before selected-frame persistence was
added. Version 1 files open with frame 0 selected. Unknown versions, invalid
button values, out-of-range stick values, incomplete frame data, excessive frame
counts, and trailing unexpected tokens are rejected.

A failed load does not modify the project currently open in memory. Saving first
writes a temporary file and only then replaces the destination.

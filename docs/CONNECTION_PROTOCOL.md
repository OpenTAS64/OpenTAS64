# OpenTAS64 Connection Protocol

## Transport and delivery

The desktop application connects to the Mupen64Plus input plugin over loopback TCP on `127.0.0.1:6464`. Protocol v3 retains the v2 one-frame-at-a-time lockstep input delivery and adds a reset control packet.

The plugin registers Mupen64Plus' `M64CMD_SET_FRAME_CALLBACK` callback. Controller-0 `GetKeys` latches the queued input and keeps it active for repeated reads during that emulated frame. The editor receives the frame acknowledgement only when the core invokes its frame callback at the next emulated frame boundary. This confirms core-frame progression; it does not claim that the host display has presented the image.

## Frame packet (18 bytes)

| Offset | Bytes | Field |
|---:|---:|---|
| 0 | 4 | `OT64` magic |
| 4 | 1 | Protocol version, `2` (frame packet layout retained) |
| 5 | 1 | Controller port, 0 through 3 |
| 6 | 8 | TAS frame number, unsigned big-endian |
| 14 | 2 | Button bitmask, big-endian |
| 16 | 1 | Signed analog X |
| 17 | 1 | Signed analog Y |

## Reset command

The editor sends the four-byte ASCII packet `RST2`. The plugin calls the Mupen64Plus Core API `CoreDoCommand(M64CMD_RESET, 1, nullptr)` and replies with five bytes: `RACK` followed by status byte `0` for success or `1` for failure.

## Rewind behavior and limitations

Rewind is implemented as deterministic replay rather than per-frame savestates: the editor requests a hard emulator reset, then replays the project's recorded inputs from frame zero until the selected timeline position. The project input data is not modified. This can take longer than a savestate rewind, especially for distant frames.

Core reset is issued from the plugin's network bridge thread. Runtime behavior depends on the Mupen64Plus build and its thread-safety guarantees; verify restart and replay with the target emulator before relying on it for a recording. Frame acknowledgements now come from the Mupen64Plus core frame callback rather than directly from `GetKeys`. The callback is a core emulation-frame boundary, not a guarantee that the host display has presented the corresponding image. If the core does not accept callback registration, the plugin refuses to start rather than silently falling back to weaker input-poll acknowledgements.

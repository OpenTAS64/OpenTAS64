# OpenTAS64 Connection Protocol

## Topology

The desktop application connects to the Mupen64Plus input plugin over loopback
TCP. The plugin binds only to `127.0.0.1` on port `6464`; it is not a LAN
control service. Mupen64Plus polls the plugin through its normal controller API.

## Version 1 packet

All packets are exactly 10 bytes and may be sent back-to-back on one TCP stream.

| Offset | Bytes | Field |
|---:|---:|---|
| 0 | 4 | `OT64` magic |
| 4 | 1 | Protocol version, currently 1 |
| 5 | 1 | Controller port, 0 through 3 |
| 6 | 2 | Button bitmask, big-endian |
| 8 | 1 | Signed analog X |
| 9 | 1 | Signed analog Y |

Button bit positions:

| Bit | N64 control |
|---:|---|
| 0–3 | D-right, D-left, D-down, D-up |
| 4–7 | Start, Z, B, A |
| 8–11 | C-right, C-left, C-down, C-up |
| 12–13 | R, L |

The plugin stores the latest state per controller. A disconnected client causes
all inputs to return to neutral. Unknown packet versions and invalid controller
indices are ignored.

## Timing limitation

Version 1 is a live-state transport only. It has no frame counter, queue,
acknowledgement, or clock synchronization. It must not yet be described as
frame-accurate TAS playback. A later protocol revision will add sequence/frame
numbers and explicit synchronization.

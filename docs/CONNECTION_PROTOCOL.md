# OpenTAS64 Connection Protocol

## Transport and delivery

The desktop application connects to the Mupen64Plus input plugin over loopback TCP on `127.0.0.1:6464`. Protocol v2 uses strict lockstep: the editor sends one input frame and waits for the plugin's acknowledgement before sending the next frame. No frame backlog is allowed.

The same request/acknowledgement mechanism is used by normal lockstep playback and the **Step Frame** control. The plugin waits in the controller-0 `GetKeys` callback when no frame is queued. The editor's pause request stops scheduling new frames and allows an in-flight frame acknowledgement to finish; Mupen64Plus then waits at the next controller-0 input poll until another frame is sent. This is an input-poll gate, not a general emulator pause API, and is awaiting runtime verification. Step Frame sends one input frame and waits for one input-poll acknowledgement before completing the step.

## Frame packet (18 bytes)

| Offset | Bytes | Field |
|---:|---:|---|
| 0 | 4 | `OT64` magic |
| 4 | 1 | Protocol version, `2` |
| 5 | 1 | Controller port, 0 through 3 |
| 6 | 8 | TAS frame number, unsigned big-endian |
| 14 | 2 | Button bitmask, big-endian |
| 16 | 1 | Signed analog X |
| 17 | 1 | Signed analog Y |

Button bit positions: 0–3 D-right/left/down/up; 4–7 Start/Z/B/A; 8–11 C-right/left/down/up; 12–13 R/L.

## Acknowledgement (14 bytes)

| Offset | Bytes | Field |
|---:|---:|---|
| 0 | 4 | `ACK2` magic |
| 4 | 1 | Protocol version, `2` |
| 5 | 1 | Status, `0` = accepted |
| 6 | 8 | Acknowledged TAS frame number, unsigned big-endian |

## Meaning and limitations

The acknowledgement is emitted only after Mupen64Plus calls the input plugin's `GetKeys` callback and that callback latches the queued input for the requested controller. This confirms that the emulator input-poll callback consumed that frame before the editor advances to the next one. After the callback returns, the core may progress until its next controller-0 poll, where it waits for the next queued frame.

For **Step Frame**, one successful acknowledgement completes one input-poll step. It does not mean that a complete rendered video frame has elapsed.

This is an input-poll acknowledgement, **not** proof that the corresponding video frame has finished rendering or that the emulator's internal frame counter equals the TAS frame number. The standard input-plugin callback API does not expose a reliable post-render frame-complete event. Exact rendered-frame synchronization will require a cooperating core/video-plugin hook or a supported emulator-side frame callback.

Only one frame may be in flight. Pausing takes effect at the next controller-0 input poll; it is not a host-level pause command and does not guarantee a rendered-frame boundary. Disconnecting clears pending input, releases all controller states, and wakes a callback that was waiting for a network frame. The emulator can then continue with neutral input; a later editor connection can resume frame delivery. Closing the ROM also clears pending input and returns controller states to neutral. Protocol v1 is not compatible with v2.

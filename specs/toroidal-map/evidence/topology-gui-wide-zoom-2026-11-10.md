# Wide zoom with repeated map copies

On 2026-11-10, in the same ordinary local game and fixture as the
[mouse-route check](topology-gui-mouse-route-2026-11-09.md), eight Keypad Minus
presses reduced the map zoom. Multiple horizontal and vertical copies of the
36×36 map became visible. The same hero, towns, fog and route markings were
visible at corresponding positions in the copies.

A click selected a destination and displayed a route on the visible copies.
A second click one horizontal map period away started movement along that
route; the server log records successful movement packets, including a wrapped
step. `AdventureMapInterface` starts the stored path on a second click only
when the destination matches, so this observation supports equivalent
selection for this one pair of horizontally repeated positions. The hero ran
out of movement points before reaching the distant destination. Zoom was
reset using Backspace after the observation.

The local F3 screenshots are:

- `screenshot-20261110T000359.png`, selected route: SHA-256
  `d8c41ff24e3e70dd22f49444dcff3f254a6ad149716c316cccf2a3d3e3bc6895`.
- `screenshot-20261110T000411.png`, after movement: SHA-256
  `33ec2db4bfe51a92c46f32cb26728c61c07dc5fe9126fac19f13434755c306e0`.

This is one camera position, one zoom and one target pair. It does not verify
vertical-copy selection, hover, every sprite/overlay, or continuous animation.
The app label and dynamic-library identity limitations are those recorded in
the linked mouse-route check; the local trace is `runtime/user-logs/VCMI_Client_log.txt`.

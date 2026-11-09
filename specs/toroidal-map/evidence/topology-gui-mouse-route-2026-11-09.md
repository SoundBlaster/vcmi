# Mouse-selected route across both seams

In the same local game and fixture described in the
[corner record](topology-gui-corner-2026-11-09.md), the camera had been
scrolled around the corner and the hero was on visitable tile `(0,0,0)`.
Using computer use, a click selected a visible grass tile, displaying a green
route, and a second click at the same location started movement. The hero
arrived in the opposite corner area on the minimap.

The integrated server log records three successful `TryMoveHero` packets,
with anchors `(1,0,0) -> (36,0,0) -> (35,0,0) -> (34,35,0)`. Thus the
visitable tiles were `(0,0,0) -> (35,0,0) -> (34,0,0) -> (33,35,0)`;
the route crossed the horizontal seam first and the vertical seam last.
The client logged each movement animation starting and ending. This is one
mouse route at one zoom, and does not establish equivalence of all repeated
map copies, hover coordinates, or frame-by-frame animation smoothness.

The game saved these local screenshots of the selected route and result:

- `screenshot-20261109T234916.png`: SHA-256
  `04b73eddb92170900e279b0ba28371536ae5fa9c02e9d337b9f8f6db33be9a0d`.
- `screenshot-20261109T234924.png`: SHA-256
  `682cf3e498e3e570bdc0614c3969e97f516b376c631d4c1b7618962660dfd0d5`.

The observed app label `1.8.0.fcc19bb` is an older build stamp and does not
identify the exact dynamically loaded libraries. Assets and screenshots are
local to TB5. The trace is in `runtime/user-logs/VCMI_Client_log.txt` on TB5.

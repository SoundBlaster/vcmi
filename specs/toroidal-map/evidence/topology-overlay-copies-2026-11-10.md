# Overlay coordinates on repeated map copies

`MapViewCache` previously passed the displayed, unwrapped tile coordinate
directly to image and text overlays. Spell-view icons compare that coordinate
to canonical object positions; the world-view icon lookup also checks an
object's visitable mask at the supplied coordinate. A repeated copy outside
the rectangle could therefore omit an icon. Text labels used the displayed
x-coordinate to choose their vertical offset: on a map with an odd width,
the same label could change offset when the camera crossed a seam.

The cache now maps each displayed tile to the canonical tile before asking
for overlay content, and uses that same x-coordinate for label placement.
It still places the resulting overlay at the displayed tile's screen position.
Bounded out-of-map coordinates retain their previous value. This applies to
the existing software render path and to the GPU path when overlays require
their own layer.

`RepeatedCopiesUseTheSameOverlayTileAndLabelParity` checks several horizontal,
vertical, and corner copies on a 5×7 two-level torus, including multiple
periods, invalid level and bounded behavior. `WorldViewObjectQueryUsesCanonicalOverlayTile`
checks a real object mask at repeated positions: the raw displayed tile is
not visitable, while the mapped tile is. These focused checks exercise the
coordinate mapping and object query; they do not instantiate the complete
renderer or establish visual continuity in a running game.

On macOS arm64, the client, main test target and standalone topology target
built. Nineteen topology tests and fifteen map draw-order tests passed; the
draw-order suite also reported one existing disabled regression. Logs are
`overlay-copies-build.log`, `overlay-copies-tests.log`,
`overlay-object-build.log`, and `overlay-object-tests.log` under the TB5
build root.

## Running-game comparison

On macOS arm64, the same 36×36 `Toroid Seam Laboratory` saved game was
loaded in signed development builds `4255569` (before) and `0043287`
(after). In each, the world view was opened with `v` at its initial zoom.
The view showed several copies of the same revealed area on both axes.
Before the change, the hero, town flag, wood and ore overlay icons appeared
on individual copies but were absent on the corresponding other copies.
After the change, those icons appeared at the corresponding locations on
the visible copies. Both clients returned to the adventure view and quit
without saving. The newer build loaded the game without a new disk-access
prompt during this observation.

The game's F3 screenshots stay in the local TB5 runtime, since they contain
original game assets; they are not committed:

| Build | Screenshot under `runtime/user-data/extracted/screenshots/` | SHA-256 |
| --- | --- | --- |
| `4255569` | `screenshot-20261110T100314.png` | `897afd73af2f950c8f09fd27689eebe435039a1578dc939a3d63637d21cea04d` |
| `0043287` | `screenshot-20261110T100105.png` | `67109693098a695b8f5ff82cc0802678054d3553331beb7068cd8135fb80319f` |

These are static world-view observations, not a continuous-scroll or
animation test. Spell-view icons, label placement in the GUI, other overlay
types, different zoom levels and maps remain unverified.

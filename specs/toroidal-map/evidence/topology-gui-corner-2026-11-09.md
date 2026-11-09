# Corner and vertical movement in the running game

On 2026-11-09, the ordinary locally built macOS client was operated using
computer-use keyboard input and visual inspection. The app's version label
was `1.8.0.fcc19bb`; this label reflects an earlier build stamp and does not
identify the exact dynamic libraries loaded in this later session.

The local 36×36×2 `Toroidal Corner Hero Fixture` placed the red hero at
visitable `(0,0,0)` on a flat surface. The player pressed Keypad 7 and the
minimap marker appeared in the opposite corner, then Keypad 3 returned it.
The client requested `(35,35,0)` and then `(0,0,0)`; the server applied
`TryMoveHero` packets with object anchors `(1,0,0) -> (36,35,0) -> (1,0,0)`.
The anchor is offset by one x coordinate from the hero's visitable tile.

From `(0,0,0)`, Up moved to `(0,35,0)` and Down returned to `(0,0,0)`;
the server applied both moves and the minimap marker showed the corresponding
north/south positions. These are single-player checks using one fixture and
one corner, not all terrain, diagonal, multiplayer or AI cases.

The fixture SHA-256 is
`860ebc5396dc9eba66c1cf70f012af0f7524ee3801b7dd5a311d9168ab6f75d1`.
Local screenshots from before and after the diagonal crossing, respectively:

- `screenshot-20261109T232237.png`: SHA-256
  `212b1f950dd020c576c39d13e855458aa6abaee382fbfca54048901ad5dc064f`.
- `screenshot-20261109T232246.png`: SHA-256
  `604f9bcd8b6a331d956eb891629caae42a3b01262e1d03e780546383dd6824cb`.

The fixture and game assets remain local. The screenshots are discrete states
and do not prove continuity of every frame of movement or scrolling.

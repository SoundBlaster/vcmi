# Directional movement in the running game

On 2026-11-09, an ordinary locally built macOS client was operated through
computer-use keyboard input and visual inspection. Its version label was
`1.8.0.fcc19bb`; the built client included the directional shortcut correction
subsequently committed as `e0466e9`. The local, uncommitted 36×36×2
`Toroidal Directional Fixture` was derived from the existing demo, with flat
surface terrain and a start position arranged for this check. Its SHA-256 was
`c26152c7a0021e91593e6dceda5a43c3934cc738abe5aec33db3c78bd6b1be5d`.
The fixture and original game assets are local only.

The human player moved the red hero to visitable tile `(0,9,0)`, pressed Left,
and observed the hero and minimap marker on the other edge. Pressing Right
returned the marker to the first edge. The client log records requested tiles
`(35,9,0)` and `(0,9,0)` and server-applied moves with object anchors
`(1,9,0) -> (36,9,0) -> (1,9,0)`. These anchors are offset from the hero's
visitable tile. The server logged successful `TryMoveHero` application; the
client logged the start and end of movement animation.

Screenshots saved on the local TB5 user-data tree:

- `screenshot-20261109T225638.png`, before crossing: SHA-256
  `8efc85cad347c2352d96b307d44fbe0ce64a9a8c06e991f0d6890e5ff5d7d106`.
- `screenshot-20261109T225655.png`, after crossing: SHA-256
  `44da7fc0cba2803e3342cdc2dab5c4bbe537e0dfaedc7ec318d260f6113a2a2c`.

After ending the turn, a real Nullkiller2 opponent used wrapped paths. Its log
reported a hero moving from visitable `(30,35,0)` to `(29,0,0)`, and another
from `(35,2,0)` to `(0,3,0)`. The latter was applied to server and client game
state; the AI reported success and subsequently attacked the red hero. This
is evidence for these particular AI actions, not support for every AI profile
or gameplay situation.

These are discrete observations, not a recording of every animation frame.
They do not establish continuous scrolling, boat animation, corner crossing,
mouse targeting, or behavior with obstacles and other AI profiles.

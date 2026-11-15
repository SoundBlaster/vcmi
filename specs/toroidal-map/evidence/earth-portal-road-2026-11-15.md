# Earth draft: experimental road to a water monolith

`MapFormat.PreparesEarthPortalRoadFixture` (test committed as `b4b5981`)
creates an opt-in local variant of the Earth draft. It removes two specifically
identified non-visitable decorations, `yuccaTrees_7568` and `mountain_7569`,
and asks the editor to draw ten dirt-road cells between the existing road at
`(2,425,0)`, the horizontal seam and the southern approach to
`monolithTwoWay_5267` at `(250,429,0)`. The test checks the removed objects,
selected tiles, portal approach direction, and saved/reloaded road types.
The original map is not changed. On macOS arm64 the test passed using the
isolated HotA profile and a build based on `2978d52` with the test in the
worktree. Its output, `earth-portal-road-qa.vmap`, is local under TB5
`runtime/earth-map-research-2026-11-15/draft-stacked/`, SHA-256
`182c1fff77286356bc2b139eb18f68bc1f1f403e8636622d68a61787edae368f`.

In a first graphical run with an ordinary staged starting hero, the route
reached `(0,428,0)` but triggered a guard battle. The game's automatic battle
ended in defeat. A second local graphical fixture, SHA-256
`b013f26fe3e221c169b54a4d46acd8a01fd88c6584a0dd506d1d4f688624e481`, staged
the same hero with 500 archers; this is a test-only advantage, not demo balance.
The threatened neutral stack fled when allowed to do so. The hero continued
along the road via `(0,429,0)`, `(0,430,0)`, `(251,430,0)` and `(250,430,0)`.
Those four cardinal steps each cost 75 movement points.

From `(250,430,0)` the hero entered the monolith. The server attempted a
teleport from anchor `(251,429,0)` to `(170,285,0)` and rejected it with
`Cannot move hero, destination tile is on water!`. The hero remained at the
entry's visitable cell `(250,429,0)`. Both graphical client runs shut down
with exit code 0; this does not verify successful portal traversal.

Read-only inspection of serialized map data found four monoliths with subtype
`hotaPortalWaterBlue`: the visited entrance is on sand, while the other three
visitable cells are on water. The attempted exit corresponds to one of those
water cells. This mismatch is a concrete demo-map issue to investigate; the
evidence does not establish whether it originates in the source map, the
conversion or a separate portal rule. The road drawing also produces a dense
ladder-like pattern next to the original road in a static view and needs
design work before use as a visual demo.

The strong-fixture observation and client log are local under TB5
`runtime/earth-portal-strong-gui-profile/`, SHA-256 respectively
`1f4d316f10c8d920cbac757db0f3e72807fe7c416abd322a969cb49d34bbd04a` and
`3463b4d6bee196197668cf18fbedf830ba0d870a2aa66d83000073248374873f`.
The isolated fixtures do not establish map balance, complete scenario
compatibility, normal unassisted play or animation continuity.

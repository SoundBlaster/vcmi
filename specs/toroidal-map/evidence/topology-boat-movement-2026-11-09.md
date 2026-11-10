# Boat movement across a seam

On 2026-11-10 a toroidal SoD fixture at revision `81bc7ab` demonstrated that
the server let a hero in a normal sailing boat use an `AIR` step directly
across a seam when the hero had Flying, although the pathfinder does not
allow a direct `SAIL` to `AIR` transition. The regression failed before the
fix. Standard toroidal movement now also rejects a direct `SAIL` to `WATER`
transition while the hero remains in that boat. The test checks both layers
against the pathfinder and confirms that sailing the same step remains legal
with matching movement cost. The corner embark/disembark, corner sailing and
aviation-vehicle embark tests also passed (`sail-flight-before-test.log`,
`sail-flight-after-build.log`, `sail-flight-after-test.log` on TB5). The
`WATER` case was added after the initial failing run, so that run demonstrates
the `AIR` discrepancy only. Other vehicles and multiplayer remain unverified.
The full local suite passed 1,392 tests, with three disabled
(`full-suite-sail-flight.log`).

On 2026-11-10 an in-memory SoD fixture placed Favorable Winds at `(0,0,0)`
before game initialization, with its footprint crossing both seams. At source
revision `e966ad1`, the toroidal test failed for all three opposite-edge and
corner cells; the bounded control passed. Initialization now canonicalizes
the footprint through map topology, and both cases pass together with the
existing movement-cost and sailing tests (`winds-wrap-before-test.log`,
`winds-wrap-after-build.log`, `winds-wrap-after-test.log` on TB5). A later
test used the initialized wind area for a westward sailing step from
`(0,35,0)` to `(35,35,0)`; it verified the wind discount against a calm
copy of the source tile and matching pathfinder/server movement points
(`winds-voyage-build.log`, `winds-voyage-test.log`). These in-memory tests
do not establish authored seam-spanning map support.
The full local suite passed 1,389 tests with three disabled
(`full-suite-winds-wrap.log`).

On 2026-11-10 an SoD fixture at revision `ab458e7` plus a new test checked
coast classification with the only water tile at `(35,35,0)`: the opposite
corner and two adjoining edges were coastal with toroidal topology and not
coastal with bounded topology; the water tile and a distant tile were not
coastal. The focused coast and sailing tests passed
(`coastal-corner-build.log`, `coastal-corner-test.log` on TB5). This checks
the map query used by battle setup and ambient sound, not those effects
themselves.

On 2026-11-10 the sailing fixture at revision `d163185` was extended to
cross each axis in both directions and each of the four diagonal corner
directions. For each step the pathfinder chose the direct same-turn route,
and server position and remaining movement matched it. This test and the
corner embark/disembark and blocked-corner tests passed on macOS arm64 with
Complete data (`boat-all-seams-build.log`, `boat-all-seams-test.log` on TB5).
This uses one synthetic corner layout and does not establish animation or
all coast configurations. The same build produced `vcmiclient` and
`vcmiserver`; it was not launched in the GUI.

On 2026-11-09 on macOS arm64 with copied Complete data, an SoD fixture
tested a hero embarking across the horizontal boundary, sailing back across
it, and disembarking across it. Before each server movement, the pathfinder
found a same-turn route, and remaining movement points matched the server's
result. The test changes the two tiles' terrain between steps to isolate the
three transitions, so it is not a playthrough of a fixed authored water map.

`vcmitest` built (`boat-seam-build.log`); from `cmake/bin`,
`./vcmitest --gtest_filter=HeroMovementTest.*:MovementCostTest.*` passed 16
tests (`boat-seam-suite.log`). Boat animation, other directions and corners,
blocked shores, AI and multiplayer remain unverified.

Source baseline `985bb2c` with local test change:

```text
fc385d4e44ccafac4db998805aae18e1683a9ef85a59fd07596c6792ef0ffd41  test/game/HeroMovementTest.cpp
```

On 2026-11-10, a Release build from `991142a` plus the new test passed
`HeroMovementTest.BoatCanSailAcrossToroidalCornerAndBothAxes` and the earlier
embark/sail/disembark test on macOS arm64 with the copied Complete data
(`boat-corner-build.log`, `boat-corner-tests.log` on TB5). On a separately
constructed SoD fixture the hero embarked without crossing a seam and sailed
`(0,0,0) -> (35,35,0) -> (0,35,0) -> (0,0,0)`. For every step the pathfinder
found a direct same-turn predecessor and its remaining movement points matched
the server result. This covers one corner and one direction on each axis, not
corner embarking or disembarking, blocked shores, AI or visual animation.

On 2026-11-10, a build from `ab55639` plus the new test passed
`HeroMovementTest.HeroCanEmbarkAndDisembarkAcrossToroidalCorner` and the two
previously mentioned boat tests (three passed). From `cmake/bin`, the
`vcmitest` filter named those three tests. On an SoD fixture the hero embarked
from `(0,0,0)` onto a boat at `(35,35,0)` and disembarked back onto the same
shore; both steps had direct same-turn pathfinder predecessors and matching
server movement points. The build log is `boat-corner-embark-build.log` on TB5.
Other corners, blocked shores and visual animation remain unverified.

On 2026-11-10, `HeroMovementTest.BoatCannotCutAcrossLandAtToroidalCorner`
exposed a pathfinder/server disagreement: both orthogonal neighbors were land,
so the sailing pathfinder rejected the diagonal `(0,0,0) -> (35,35,0)`, but
the server accepted it and spent movement points. The server now reuses the
pathfinder neighbor calculation for standard sailing on toroidal maps. The
same test then passed, confirming the hero, boat and movement points remain
unchanged, alongside three legal boat-transition tests and both movement-cost
tests (six passed). `vcmitest`, `vcmiserver` and `vcmiclient` built from
`c8de5ec` plus this change (`boat-blocked-corner-after-build.log` on TB5).
This does not change the server's bounded-map behavior or establish all shore
configurations, client animation, or multiplayer behavior.
The full local `vcmitest` run after this change passed 1,372 tests from 224
suites, with three disabled (`full-suite-boat-coast-fix.log` on TB5).

On 2026-11-10, a hero on foot could request `SAIL` on a land-to-land seam
step, and the server accepted it. Standard toroidal movement now requires a
matching sailing vehicle or a non-transit embark onto one for that layer.
The new test verifies the invalid request leaves the hero and points unchanged
and that a subsequent `LAND` step succeeds. It passed with five legal and
blocked boat/aviation scenarios; the full local suite passed 1,380 tests from
224 suites, with three disabled (`full-suite-invalid-sail.log`). The build
used `0660a28` plus this change (`invalid-sail-after-build.log` on TB5).
This does not establish all vehicle types, layer transitions or multiplayer.

The blocked-corner test then exposed another bypass: a sailing hero could
request the water destination as `LAND`, causing the server to accept the
otherwise prohibited diagonal step. Standard toroidal moves now require
`LAND` destinations to be land and `SAIL`/`WATER` destinations to be water,
matching the pathfinder's layer/terrain classification. Tests verify all
three mismatches are rejected without movement or cost, alongside legal
disembarking and flight/water walking. Four focused tests and the complete
local suite passed; the latter ran 1,380 tests with three disabled
(`full-suite-layer-terrain.log` on TB5). Client, server and test targets
were built from `9112115` plus this change (`layer-terrain-after-build.log`).
Vehicle-specific rules and multiplayer remain only partially covered.

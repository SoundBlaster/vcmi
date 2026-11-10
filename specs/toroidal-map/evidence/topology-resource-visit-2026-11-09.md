# Resource interaction across a corner

On 2026-11-10 a synthetic SoD game placed a hero at `(10,0)` and a neutral,
unguarded mine with its visitable cell at `(10,35)`. The test checked that
the mine has no visible, blocking or visitable cells outside the map. On a
bounded map the direct move was rejected without changing owner or hero
position. With wrapping enabled, the pathfinder chose the direct step, and
the real server move entered the mine and changed its owner to the hero's
player. Three focused interaction and guard tests passed on macOS arm64
(`toroidal-mine-final-build.log` and `toroidal-mine-final-test.log` on TB5).
All 47 `HeroMovementTest` cases subsequently passed
(`toroidal-mine-movement-suite.log`).
The mine was created through the server in an in-memory fixture. This does
not verify GUI input, multiplayer, guarded mines, or future resource income.

On 2026-11-10 a synthetic impassable object across a seam exposed another
pathfinder/server disagreement at revision `24b52c8`: a hero with flying
ability could request `LAND` movement and enter the blocker. The regression
test failed before the fix. For standard movement on toroidal maps, the
server now permits flying over a non-visitable blocker only on the `AIR`
layer. The test also confirms that actual air transit succeeds, and all 41
`HeroMovementTest` cases passed. The full suite passed 1,386 tests with three
disabled (`full-suite-fly-land-obstacle.log`). The TB5 build root contains
`fly-land-obstacle-before-*` and `fly-land-obstacle-after-*` logs. This does
not test every object or flying-vehicle configuration.

On 2026-11-09 on macOS arm64 with copied Complete data, an SoD fixture placed
a hero at (0,0) and a gold pile at (35,35). A server-authorized diagonal
action collected 1,000 gold across the corner and removed the resource while
the hero remained on the original tile, as required for this blocking visit.
A second action moved the hero onto the vacated tile without giving the
resource a second time.

`vcmitest` built (`object-interaction-build.log`); from `cmake/bin`,
`./vcmitest --gtest_filter=HeroMovementTest.HeroCollectsResourceAcrossToroidalCorner`
passed (`object-interaction-tests.log`). This covers one resource interaction,
not guarded resources, other objects, client input, or multiplayer.

Source baseline `8d04656` with local test change:

```text
466342e4654c5d1883eef56a14c628a2eef672ae9f1cf4c151e4bcefd904a063  test/game/HeroMovementTest.cpp
```

On 2026-11-10 a synthetic seam resource with no permitted approach
directions exposed a disagreement: the pathfinder rejected the destination,
but the server accepted the move, spent movement points and awarded gold.
Standard toroidal movement on land, sailing or water-walking layers now
checks the destination's approach direction using the same helper as the
pathfinder before performing the visit. `ServerRejectsForbiddenObjectEntranceAcrossToroidalSeam`
then confirmed rejection with position, movement, gold and resource unchanged.

The broader run initially failed four existing synthetic interaction cases:
their templates had `visitableFrom: []`, which forbids every direction despite
the tests expecting successful visits. The resource and three portal fixtures
were corrected to explicitly permit all directions. The 30 movement and demo
tests then passed, followed by all 1,373 local tests from 224 suites; three
were disabled (`full-suite-entrance-fixtures.log` on TB5). The server, client
and tests were built from `0c60cc5` plus this change. This covers destination
entrance in the named scenario; source-object exit rules, other objects,
multiplayer and GUI behavior remain unverified. Bounded server behavior was
not changed.

A follow-up revealed that the same hero could claim the `AIR` layer without a
flying ability and bypass the entrance check. Standard toroidal moves now
reject `AIR`, `WATER`, or `AVIATE` when the hero or vehicle lacks the relevant
capability. The forbidden-entrance test covers all three requests without
movement, resource collection or movement-point changes. A separate test
confirms a hero with flying and water-walking bonuses can still cross a seam
using those layers. Four focused tests passed, followed by 1,374 local tests
from 224 suites, with three disabled (`full-suite-layer-authority.log` on
TB5). The client, server and test targets built from `47f0cb7` plus this
change (`unavailable-layer-after-build.log`). Other layer transitions and
multiplayer remain unverified; this change does not alter bounded maps.

In a further synthetic test, a hero standing on a blocking visitable object
with no permitted exit directions could not find a path across the seam, but
the server accepted that step. The server now checks the source object's
direction for standard toroidal movement when neither flying nor water walking
is available; in that case it can determine that the relevant source layer is
ordinary land or sailing. The test now confirms rejection without changing
position or movement points. The source layer is not tracked by the server,
so this check deliberately does not claim coverage when flying or water
walking is available. Thirty-seven movement, gate and demo tests passed;
the full local suite passed 1,376 tests with three disabled
(`full-suite-source-exit.log` on TB5). The build used `2bd0144` plus this
change (`source-exit-direction-after-build.log`); bounded maps are unchanged.

An additional SoD test uses a resource that forbids entry from the north but
permits it from the west. Across the north/south seam the server rejects the
forbidden step without collecting or spending movement; after a short route
the hero collects the same resource from the west across the east/west seam.
The pathfinder finds a route whose final predecessor is not the forbidden
neighbor. This test and the all-directions-forbidden entrance and exit tests
passed (three tests), using `75cd77a` plus the new test
(`directional-entrance-build.log` on TB5).

The layer validation initially also rejected embarking an aviation vehicle,
because the hero was not yet aboard. A follow-up permits a non-transit embark
when the vehicle's own layer matches the requested layer. A synthetic boat
whose layer was set to `AVIATE` can now be boarded across a seam; attempts to
transit through it or claim `AIR` without flight are still rejected without
movement or cost. Four focused tests passed, then all 1,379 local tests from
224 suites passed with three disabled (`full-suite-aviation-embark.log` on
TB5). The build used `a0609e2` plus the change
(`aviation-embark-after-build.log`); this is not a test of a specific mod,
its client presentation, or full airship path planning.

With a real flying bonus, an `AIR` request without transit still triggered a
blocking visit to the forbidden resource. The normal human client and
Nullkiller2 send transit for `AIR` movement. Standard toroidal `AIR` movement
now requires transit, except when actually boarding a matching vehicle. The
test confirms the non-transit visit is rejected and a transit flight crosses
the tile without collecting the resource. Three focused tests passed, and the
full local suite passed 1,380 tests with three disabled
(`full-suite-flight-transit.log` on TB5). Client, server and tests were built
from `43db0ef` plus this change (`flight-transit-after-build.log`). Other AI
profiles, water-walking visit semantics and multiplayer remain unverified.

A further water-walking fixture placed a blocking resource on water across a
seam. The pathfinder rejected the `WATER` destination, while the server
accepted a non-transit visit and collected it. The server now rejects blocked
`WATER` tiles for standard toroidal moves, except for boarding a matching
vehicle. The test verifies both transit and non-transit attempts leave the
hero, movement points, gold and resource unchanged. Three focused tests and
the full local suite passed; the latter ran 1,381 tests with three disabled
(`full-suite-water-blocking.log` on TB5). Client, server and tests were built
from `ad0829a` plus this change (`water-blocking-after-build.log`). Other
water-object rules, vehicle types and multiplayer remain unverified.

An additional synthetic `AVIATE` vehicle test exposed the same disagreement
for a visitable resource across a seam. The pathfinder marked the destination
unreachable, but the server triggered a visit. Standard toroidal `AVIATE`
movement now rejects blocked or visitable destinations except when boarding
a matching vehicle, following the pathfinder's tile classification. The test
checks no movement, point expenditure or resource collection and then moves
the vehicle onto a free tile. Three focused tests and the complete local suite
passed; the latter ran 1,382 tests with three disabled
(`full-suite-aviation-blocked.log` on TB5). The client, server and tests were
built from `6e3b605` plus this change (`aviation-blocked-after-build.log`).
This synthetic vehicle does not establish support for a specific mod.

The layer/terrain compatibility and blocked-object rules are now shared by
`NodeStorage`, `PathfinderUtil` and the toroidal server checks instead of
being repeated. The server retains the explicit matching-vehicle embark
exception. The complete local suite passed 1,382 tests with three disabled
(`full-suite-shared-layer-rules.log` on TB5). Client, server and test targets
were built from `d34da7f` plus the refactor (`shared-layer-rules-build.log`).
This run includes bounded-map tests but is not a complete bounded regression
or multiplayer validation.

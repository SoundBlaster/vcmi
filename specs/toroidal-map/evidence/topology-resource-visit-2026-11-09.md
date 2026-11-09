# Resource interaction across a corner

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

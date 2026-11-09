# Boat movement across a seam

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

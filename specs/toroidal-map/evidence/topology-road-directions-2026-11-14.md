# Road movement across every edge and corner

`HeroMovementTest.RoadCostsAgreeAcrossEveryToroidalEdgeAndCorner` uses a
synthetic 36×36 map with surface and underground levels. For each of the
eight directions on each level, the test places a hero at the appropriate
edge or corner and cobblestone road on both the source and destination
tiles. The shared pathfinder must choose the wrapped adjacent tile directly;
the server must then accept the same step and report the same movement cost:
50 points orthogonally and 70 diagonally.

All 16 combinations and the earlier one-edge road test passed on macOS arm64
from `961e235` plus the test change. Hero repositioning between cases is a
fixture operation; the actual steps are performed by `CGameHandler::moveHero`.
The test does not cover other road types, road rendering, graphical input,
network clients or a complete played map.

`HeroMovementTest.MixedRoadTransitionsAgreeAcrossToroidalSeams` subsequently
checks four road configurations: cobblestone to gravel, gravel to cobblestone,
cobblestone to no road and no road to cobblestone. Each is executed in both
directions across a horizontal seam and across a diagonal corner, on each of
two levels: 32 combinations. The pathfinder's direct step and the server's
applied movement cost agree. The three road tests passed on macOS arm64 from
`47ddbbd` plus the added test. Dirt roads, other diagonal corners, network and
GUI input remain outside this mixed-road check.

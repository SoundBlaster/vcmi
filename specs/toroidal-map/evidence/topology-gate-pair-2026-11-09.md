# Subterranean gate pairing across a seam

`CGSubterraneanGate::postInit` pairs visitable gates on different levels by
their horizontal distance. On a toroidal map it now compares the shortest
wrapped displacement, while retaining the existing ordering and level rules.
Bounded maps retain their previous distance calculation.

The `HeroMovementTest.subterraneanGatePairsWithNearestAcrossToroidalSeam`
regression creates synthetic gates at `(0,5,0)`, `(3,5,1)` and `(35,5,1)` on
a 36-wide map. It checks that the first gate pairs with `x=3` in bounded mode
and `x=35` in toroidal mode. The toroidal assertion failed before the change;
the focused topology, movement and Town Portal run passed all 29 tests after
it. Build and test logs are on TB5 as `gate-pair-after-build.log` and
`gate-pair-after-tests.log` under the VCMI build root.

That first test verifies assigned channel IDs only.

The later `SubterraneanGateTopologyTest.heroUsesGatePairedAcrossToroidalSeam`
initializes synthetic gate objects as bidirectional teleports and invokes the
actual pairing code. It verifies the channel connection, enters the surface
gate with a real server movement, answers the teleport dialog, and checks that
the hero appears at the underground gate across the seam. It then steps away,
re-enters that gate and verifies the return to the surface. The focused run
passed 28 tests from three suites (`gate-traversal-build.log` and
`gate-traversal-tests.log` on TB5). These are synthetic templates in a test
game state, not a GUI check of original gate graphics or all passage types.

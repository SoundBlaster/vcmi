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

## Teleport exits whose footprint crosses a seam

An exit's visitable tile can itself lie across the seam from its stored
anchor. Previously `CGTeleport::isExitPassable` asked the game state for the
raw coordinate `(-1,5,1)` and raised an exception, even though occupancy was
recorded on `(35,5,1)` (`seam-gate-before-tests.log`). Teleport exit checks,
dialog choices and random monolith destinations now explicitly resolve such
positions on toroidal maps; bounded coordinates are unchanged.

The synthetic `heroUsesGateWithVisitableTileAcrossSeam` uses the actual server
movement and dialog activity to travel to a gate whose raw visitable tile is
outside the map, then returns through it. A separate test exercises a one-way
monolith whose random exit has the same property.

Whirlpools can have several visitable exit tiles. Their exit enumeration now
resolves and deduplicates wrapped tiles before checking occupancy. The
`whirlpoolListsPassableExitTilesAcrossSeam` synthetic fixture found only one
of two tiles before the change (`seam-whirlpool-before-tests.log`); after the
change it finds both in toroidal mode and one in bounded mode. Five focused
gate, monolith and whirlpool tests passed (`seam-teleport-whirlpool-tests.log`)
and client, editor, and test targets built (`seam-teleport-whirlpool-build.log`).
The complete local suite passed 1,361 tests from 222 suites, with three
disabled (`full-suite-seam-teleports.log`).

Those fixtures used in-memory synthetic objects and bypassed the static-object
save restriction that applied at the time. That run did not verify actual
whirlpool travel, multiple-choice monolith dialogs, original artwork,
multiplayer or GUI rendering. Later work supports some seam-crossing static
objects; see the implementation matrix for its current limits.

## Boat travel through a seam-spanning whirlpool

Additional server-side tests build a protected single-creature hero and
an unprotected ten-creature hero, respectively. Each hero boards a boat,
sails into a whirlpool and arrives at the other whirlpool's `(35,5,0)`
visitable tile, whose raw coordinate lies across the seam. A second boat
occupies the other exit tile, so the selectable or random exit must be the
unoccupied one. After the teleport the tests verify the same boat and hero
occupy the destination, the entrance no longer contains the hero, and the
teleport does not consume additional movement points. The protected army
remains intact; the unprotected stack loses five of ten creatures. A third
case occupies both exits: the protected hero and boat stay on the entrance
tile with no further movement cost.

All seven tests in `SubterraneanGateTopologyTest` passed on macOS arm64 using
the TB5 build from `f97bf21` plus these tests. These are synthetic whirlpools
and in-process server activities. Original artwork, the GUI dialog, multiple available random exits and
multiplayer remain
unverified.

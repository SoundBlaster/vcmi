# Object tile queries across seams

`CGObjectInstance::visitableAt`, `blockingAt`, `coveringAt` and `drawLayerAt`
previously subtracted the queried coordinate directly from the object anchor.
They now translate a canonical map tile to the object's local footprint on a
toroidal map when a direct footprint lookup does not fit. The direct path and
bounded behavior are preserved. Out-of-map input is not newly wrapped: some
callers intentionally use raw footprint coordinates and others use invalid
positions. Oversized footprints are excluded from the new conversion.

`HeroMovementTest.ObjectTileQueriesFindFootprintAcrossToroidalCorner` uses a
two-level game state and a synthetic 2×2 object anchored at `(0,0,0)`. It
checks all four queries at the raw footprint tile `(-1,-1,0)` and the
canonical opposite corner `(35,35,0)`, compares bounded and toroidal modes,
and checks an outside position and an invalid level. The toroidal expectations
failed before the fix. A focused run passed 35 tests in object draw ordering,
movement, gate and boat suites; `vcmitest`, `vcmiclient` and `vcmieditor`
built successfully. Logs on TB5: `object-tile-queries-before-tests.log`,
`object-tile-queries-after-build.log`, `object-tile-queries-after-tests.log`.
The complete local suite passed 1,356 tests with three disabled
(`full-suite-object-tile-queries.log`); it preceded an additional focused
assertion checking behavior when the object has no callback.

This is groundwork, not permission to author static objects crossing a seam.
Other object coordinates, interactions, oversized masks and actual sprite
rendering remain unverified; the existing load/save restriction stays active.

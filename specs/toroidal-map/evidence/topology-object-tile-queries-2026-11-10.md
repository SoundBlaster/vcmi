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

The subsequent `HeroMovementTest.SeamSpanningBlockerPreventsMovementOnBothSides`
uses a synthetic two-tile blocking object across the east/west seam. Both
canonical tiles report blocked, the pathfinder marks one unreachable, and the
server rejects entry from each neighboring side. Both this test and the tile
query test passed (`seam-blocker-tests.log`). It uses an in-memory fixture
that bypasses the authored-map restriction; it is not a test of forest art,
random template generation, or a diagonal opening around the blocker.

`HeroMovementTest.CollectsResourceWithVisitableTileAcrossSeam` moves an actual
resource onto a synthetic footprint whose raw visitable coordinate is
`(-1,10,0)` and whose canonical tile is `(35,10,0)`. The game-state occupancy
identifies the resource at the canonical tile, and a server movement into it
collects and removes the resource. The broader related run passed 36 tests
(`seam-visitable-related-tests.log`). `visitablePos()` itself still returns the
raw coordinate in this case; a global change would need to account for object
construction before the editor has attached a map. The single resource check
does not prove that other consumers handle this raw value, and the authored
static-object restriction remains in force.

The later unbound-callback check exposed a regression in the new footprint
lookup: `EditorCallback` throws if asked for a map header before a map is
attached. `IGameInfoCallback::tryGetMapHeader` now allows callbacks with an
unattached state to provide a nullable result; `EditorCallback` does so, and
the footprint and boat-generator queries use it. The test checks both the
unattached fallback and wrapped lookup after the callback is attached. It
failed before this correction (`unbound-editor-before-test.log`).

After rebuilding `vcmitest`, `vcmiclient`, and `vcmieditor`, the complete
suite passed 1,358 tests from 222 suites, with three disabled
(`full-suite-unbound-editor-fix.log`). Adding a virtual callback method changes
the C++ interface; this local test does not establish ABI compatibility with
separately built plugins or mixed versions. The [GUI conversion check](topology-legacy-editor-2026-11-10.md)
used the earlier editor build, before this correction.

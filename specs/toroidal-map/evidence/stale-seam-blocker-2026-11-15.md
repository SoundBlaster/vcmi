# A stale movement step after a blocker moves

`HeroMovementTest.StaleSeamStepIsRejectedAfterBlockingObjectMovesIntoTheWay`
first computes a direct on-foot path across each of the four edges and four
corners of a fully revealed 36×36 toroidal map, on both levels. It then uses
the real server `changeObjPos` API to move a synthetic non-visitable blocking
object onto the destination after the path was calculated. The original
movement request is rejected without changing the hero position or movement
points. A fresh pathfinding search also finds the cell unreachable. After the
object moves away, the same step succeeds at the expected ordinary or diagonal
cost.

This checks server revalidation after an in-process state change and one kind
of blocker. The test uses a resource object with a synthetic blocking-only
appearance; it does not represent a particular mod, a TCP delivery, GUI path
invalidation, Flight or Water Walk, hidden terrain, or another hero blocking
the destination. It does not establish seamless animation.

On macOS arm64, `vcmitest` built from `9af481b` plus this test. The new test,
the prior movement/guard-cache cases, and the Water Walk route-agreement case
passed: 11 tests total. The report is
`runtime/stale-seam-blocker-regression-2026-11-15.xml` under the TB5 build root.
The build emitted duplicate-library and missing Conan search-path warnings.

# Guard cache after hero movement and removal

The guard cache can depend on the directional approach mask of a blocking,
visitable object at the guarded tile. A synthetic hero with such an appearance
was teleported onto a tile adjacent to a monster. On both bounded and
toroidal maps, and on both levels, the direct guard calculation then found no
guard while the cached callback still returned the monster. The toroidal case
placed the monster and target on opposite sides of the north-south seam.
After fixing movement, removing that hero exposed the reverse stale-cache
result: the direct calculation saw the guard again, while the cache did not.
All four parameter combinations failed before each corresponding correction.

`GameStatePackVisitor` now recomputes the cells in the old and new hero
footprints when a movement packet changes the hero's position. It also
refreshes the affected boat footprint when embarking, disembarking or moving
with a boat. When a defeated hero is removed, the hero and associated boat
footprints are refreshed after removal. A monster itself cannot move through
these hero packet branches, so only affected footprint cells need updating.

The synthetic `TryMoveHero` teleport and `RemoveObject` packets exercise
arrival, departure and removal using a custom directional hero appearance.
They pass on both topologies and levels. The focused set of 32 tests also
covers existing monster guard, boat movement and two-peer TCP scenarios.
Reports before and after the corrections are under the TB5 build root in
`runtime/hero-guard-cache-before-2026-11-15.xml`,
`runtime/hero-guard-removal-before-2026-11-15.xml` and
`runtime/hero-guard-cache-final-2026-11-15.xml`.

With both fixes, the complete local macOS arm64 `vcmitest` run passed 1,973
tests; five external-fixture or external-server tests were skipped and three
tests remained disabled. Its log and XML are
`runtime/full-suite-hero-guard-final-2026-11-15.{log,xml}` on the TB5 build
root. The build succeeded with duplicate-library and missing Conan
search-path warnings.

These tests apply synthetic packets to an in-process game state. They do not
establish that a stock hero uses the restrictive mask, that a particular mod
does, or that all boat masks and GUI paths have been exercised. The change
also affects bounded maps, and it adds guard-cache recomputation during hero
movement; performance under very large armies of moving heroes is unmeasured.

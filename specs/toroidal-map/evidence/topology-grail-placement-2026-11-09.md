# Grail placement on toroidal maps

When choosing a random Grail location, toroidal maps no longer exclude a
9-tile border and a specified radius uses shortest wrapped horizontal
distance. The existing 2D comparison still ignores level differences. An
unspecified location (`x < 0`) has no center, so every otherwise eligible
tile is considered; the sentinel is never wrapped into a real position.
Bounded maps retain their previous behavior.

The `GrailTopologyTest` fixture modifies the loaded map before game-state
initialization so only `(35,10,0)` is eligible. With center `(0,10,0)` and
radius one, it verifies that a bounded map cannot select the border tile and
a toroidal map selects it across the seam. A separate toroidal case checks an
unspecified center. Both toroidal tests failed before the change; all three
passed afterward. The focused run passed 39 tests across five suites; logs
`grail-before-tests.log`, `grail-after-build.log` and `grail-after-tests.log`
are on the TB5 build root.

On 2026-11-10, `HeroMovementTest.grailCanBeDugAfterCrossingToroidalCorner`
used the real server move and dig handlers on a single-level SoD fixture.
The hero crossed from `(0,0,0)` to `(35,35,0)`, where the test explicitly
placed the Grail, then dug with full movement points. The hero received the
Grail, spent its movement and left a hole. An in-memory game-state snapshot
and reload retained the toroidal flag, hero position, artifact, movement
points and hole; a second dig on the original state was rejected. The test
and the three placement tests passed together from `fa087c6` plus this test
on the macOS arm64 Release build. This is not an on-disk save test or a GUI
observation, and does not combine random placement with digging.

Puzzle-map rendering, other levels, map editor authoring, and statistical
distribution of random placement remain unverified.

## Wrapped holes

On 2026-11-10, a synthetic map placed a hole at raw anchor `(-1,10,0)` and
made `(35,10,0)` the only eligible toroidal Grail location. Before the fix,
the game chose the occupied location and `isHoleAt` failed to recognize the
wrapped hole. Grail placement now canonicalizes hole positions before
excluding them. On a toroidal map, hole lookup compares canonical positions;
bounded lookup and the serialized raw positions remain unchanged. A direct
canonical lookup is used first, followed by a scan of raw hole positions when
needed; performance with unusually many holes has not been measured.

`GrailTopologyTest.WrappedHoleIsNotAnEligibleGrailLocation` verifies the
exclusion, dig status, invalid-level isolation and in-memory game-state
restore. `MapObjectDrawOrderTest.HoleLookupWrapsAndTracksHiddenObjects`
checks bounded and toroidal lookup, two raw positions that map to the same
tile, and hiding. Those cases and four related Grail tests passed. The macOS
arm64 client, server, editor and tests built; the full suite passed 1,437,
skipped three optional external-input tests and reported five disabled tests
(`full-suite-wrapped-grail-hole.log` on TB5). Existing linker warnings remain.
The synthetic hole does not prove behavior for every legacy map or an on-disk
save containing this case.

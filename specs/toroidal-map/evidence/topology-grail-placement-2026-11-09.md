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

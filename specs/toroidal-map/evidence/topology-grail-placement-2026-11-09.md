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

This does not verify puzzle-map rendering, digging, other levels, map editor
authoring, or statistical distribution of random placement.

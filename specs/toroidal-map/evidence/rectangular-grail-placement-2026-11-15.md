# Unspecified Grail on bounded rectangular maps

The legacy Grail initialization used twice the map width as a stand-in for
an unrestricted radius from the unspecified-position sentinel. That covers
normal square maps, but can exclude valid cells near the far end of a tall
rectangle. A synthetic 23×100 bounded map with its only eligible cell at
`(11,80,0)` reproduced the issue: the Grail position remained unspecified.
The corresponding 100×23 control succeeded.

When both the center is unspecified and the radius is zero, placement now
considers every otherwise eligible tile. The existing bounded border
exclusion and explicitly specified radius remain in force. This is a legacy
bounded-map fix independent of the toroidal flag.

The tests construct a small H3M and replace its terrain grid in the loaded
map with the rectangular grid during setup. They verify both orientations,
an explicit radius and a near-border-only candidate. These four cases, four
existing Grail topology cases, and corner digging passed three consecutive
runs on macOS arm64; the XML report is
`runtime/rect-grail-final-2026-11-15.xml` under the TB5 build root. The tall
case failed before the fix and the wide case passed. The test does not cover
all layouts, editor behavior or a real rectangular VMAP. Existing duplicate
library and missing Conan search-path linker warnings were present.

The subsequent complete local suite ran 2,005 tests: 1,999 passed and six
optional tests were skipped; three tests were disabled. Its log and XML are
`runtime/full-suite-rect-grail-2026-11-15.{log,xml}` on TB5.

# Road and river brush neighborhoods

`MapManager.ToroidalLineBrushMatchesInteriorAtEdgesAndCorners` constructs all
256 combinations of the eight neighboring cells around a line tile. For each
combination it paints the same neighborhood at an interior location, each
edge and each corner of a 12×12 toroidal map, on both map levels. It repeats
the comparison for dirt roads and water rivers. The test compares the selected
sprite group and reflection flags; it permits random differences between
artwork variants within a group. It does not compare rendered pixels.

On macOS arm64, the test, the existing edge-turn and river-flag regressions,
and the visual fixture round-trip test passed in three consecutive runs. The
log is `runtime/line-neighborhoods-all-corners-tests.log` under the TB5 build
root. With the prototype VMAP and save and the Complete H3M fixture supplied,
the full local suite passed 1,663 tests; four were disabled. Its log is
`runtime/full-suite-line-neighborhoods-2026-11-15.log`.

This is an editor operation check, not evidence of seamless scrolling,
interactive Qt painting, existing-map repair, or every road or river artwork
type. The separately recorded static visual observation covers one dirt-road
seam on each axis.

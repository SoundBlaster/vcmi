# Rectangular toroidal VMAP movement

`BothAspectRatios/ToroidalRectangularVmapTest.ServerAndPathfinderCrossEverySeamOnBothLevels`
constructs two synthetic maps, `36×23` and `23×36`, with grass on two levels.
The test starts from a square H3M fixture, replaces its physical terrain grid
with the rectangular grid, rebuilds object occupancy with toroidal topology,
and serializes it as a VMAP in memory. A real VMAP header and map loader then
initialize a game state. The header and initialized map retain the distinct
width and height and the toroidal flag.

For each aspect ratio, the fixture relocates the hero during setup to each
edge and corner on both levels. Across all eight directions, the pathfinder
finds the direct wrapped neighbor and the server movement handler reaches it
with the movement points predicted by the pathfinder. The two aspect-ratio
cases and four minimum-size tests passed three consecutive local runs on
macOS arm64.

The complete local suite with the prototype VMAP and save plus the Complete
`Dead and Buried` H3M passed 1,653 tests with four disabled. The log is
`runtime/full-suite-rectangular-vmap-2026-11-15.log` under the TB5 build root.
This is local evidence, not hosted CI.

The initial setup used revealed, empty grass and synthetic hero relocations;
it did not establish visual rendering, road art, other terrain, object
interaction, save-game reload, AI or network play for arbitrary rectangular
maps.

## Playable rectangular fixtures

The fixture now includes two players, a gold pile across the northwest
corner from the first hero, and dirt roads drawn across both axes. With
`VCMI_TOROID_RECT_OUTPUT_DIR` set to an existing directory, the two
parameterized tests also write their VMAPs there, refusing to overwrite a
file. The tests load those same VMAP bytes, collect the gold through the
corner on the server, then check the pathfinder and server across every edge
and corner on both levels. Both parameter cases passed. The complete local
suite passed 1,653 tests with four disabled; its log is
`runtime/full-suite-rectangular-demo-corrected-2026-11-15.log` on TB5. An
earlier run failed the optional prototype-save test because the supplied
file was a newer tagged save; the corrected run used the original untagged
save identified by the SHA-256 in the save-marker evidence.

The generated maps are installed in the isolated TB5 Complete-data map
directory as `toroid-rectangle-36x23.vmap` and
`toroid-rectangle-23x36.vmap`, with SHA-256 respectively:

```text
a12408706d2069d038f0ee06295768d8d719b65c168f03c30e6584b77e3fbcbe
65d728846b70e13b7b97a31236eecec206512fab3b3c6fe9409394d1f70c99e2
```

In the signed `14c28fd` preview client, the 36×23 map appeared in the
scenario list with its dimensions and started successfully. The diagnostic
observation reported toroidal topology, hero position `(0,0,0)` and 20,000
gold. Two northwest commands collected 1,000 gold and moved the hero to
`(35,22,0)`; the final game screen was also inspected.

The 23×36 map was subsequently started in the same client. Its minimap
occupied the logical rectangle `[1136,26,92,144]`. Diagnostic clicks on
the four inside corner pixels, with observations read between clicks,
placed the camera centers at pixel coordinates `(16,16)`, `(720,16)`,
`(16,1136)` and `(720,1136)`, respectively. With 32-pixel tiles these are
the centers of map cells `(0,0)`, `(22,0)`, `(0,35)` and `(22,35)`. The
first northwest hero command collected 1,000 gold; the second moved the hero
from `(0,0,0)` to `(22,35,0)`. A final game screen was inspected and the
session was closed without a manual save. The client log and observation
stream are in `runtime/gui-rect-portrait-2026-11-15/` on TB5; the stream's
SHA-256 is `bcd6995c7254d4a118891d8305f3416f5a9837a1d6ffb7ab52e09cdd6ed056f7`.

These checks do not verify smooth scrolling, movement animation, the other
minimap scales or all road art on rectangular maps.

## Scenario-list size classification

The original scenario list classified, sorted and filtered maps using only
their width. The two demo maps therefore appeared as `S` and `C` respectively
despite having the same dimensions in opposite orientations. The header icon
had the same issue: the new `ScenarioMetadataTest.mapSizeIconUsesLongerDimension`
failed for 23×36 and 36×72 before the fix.

The list and map-info icon now use the longer side; dimensions remain
unchanged. A standard-sized square retains its previous classification. A
nonstandard longer side still uses the existing custom-category fallback.
The test checks 36×36, 36×23, 23×36, 36×72 and 72×36 through both the
map-info icon and scenario metadata; it passed along with the bounded-edge
and rectangular-map tests. This does not test every possible dimension or
client-side sort order.

The signed local `VCMI Toroid Rect Size Trial.app`, built from `ebeb910`
plus the fix, showed both demo maps with the `S` label after the Small size
filter and name filter were applied. The selected 23×36 map also showed the
small icon. The screenshot was observed but not saved. Its client and engine
library SHA-256 hashes were respectively
`0cff859b78e11868c3687f053483c3adfde26f15f23bcbcb75225b936d3e67d1` and
`d509149cc240baa90880f6c8d210b3da8be3064cf08eb08b1b9a3eca47d39d5c`.
The complete local suite passed 1,654 tests with four disabled; the log is
`runtime/full-suite-rect-size-fix-2026-11-15.log` on TB5. This is not hosted
CI and does not establish graphical behavior on other platforms.

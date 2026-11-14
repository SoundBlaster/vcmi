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
`(35,22,0)`; the final game screen was also inspected. The other aspect
ratio has not been started in the GUI. This is not a verification of smooth
scrolling, movement animation or all road art on rectangular maps.

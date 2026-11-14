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

The setup uses revealed, empty grass and synthetic hero relocations; it does
not establish visual rendering, road art, other terrain, object interaction,
save-game reload, AI or network play for arbitrary rectangular maps.

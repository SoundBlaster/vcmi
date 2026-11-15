# Terrain redraw preserves independent tile flags

The editor stores terrain, river and road orientations and other state in
different bits of `TerrainTile::extTileFlags`. Rock transitions use separate
terrain images for rotation; when redrawing such a tile, the editor used to
clear the entire byte. This could discard road and river orientation and
other tile flags, including on tiles across a toroidal seam. The change clears
only the terrain orientation bits. It also applies to bounded maps.

`MapManager.TerrainRedrawPreservesRoadAndRiverFlags` initializes the upper six
bits and both terrain rotation bits, draws a subterranean patch surrounded by
rock, and checks the upper bits on every tile on a bounded map and a map with
the patch at a toroidal corner. It checks terrain rotation bits on one
redrawn rock tile, then checks preservation through undo and redo. Before the
fix, the test failed on multiple surrounding rock tiles in both map modes;
afterward it passed. The flags are synthetic; this is not a graphical editor
or in-game rendering test of roads on a playable terrain.

The macOS arm64 TB5 build of `vcmitest` succeeded, with linker warnings about
duplicate libraries and missing Conan search paths. All 11 selected tests
passed: `MapManager.*`, `MapTopology.Editor*`, and
`MapFormat.ToroidalMixedRoadTurnsRoundTrip`. The XML report is
`runtime/terrain-redraw-flags-2026-11-15.xml` under the TB5 build root.

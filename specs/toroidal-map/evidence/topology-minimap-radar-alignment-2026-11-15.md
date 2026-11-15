# Minimap radar alignment

The toroidal minimap's terrain raster and click selection use the same
pixel-to-tile mapping. The split radar rectangle instead used truncating
tile-to-pixel arithmetic. At 36 tiles displayed in 150 pixels, for example,
the old left edge for a visible interval starting at tile 3 was pixel 12,
although that pixel belongs to tile 2 in the actual raster mapping.

The radar now maps each nonempty, canonical tile interval to the half-open
pixel interval owned by those tiles, using the same mapping as the raster and
clicks. A split interval with no represented pixels is skipped. The bounded
minimap code is unchanged.

`MapTopology.ToroidalMinimapRadarMatchesDisplayedTiles` checks the specific
36/150 case and every nonempty tile interval at 36/150, 23/144, 252/144 and
3/1 against independently enumerated pixel ownership. This includes ranges
with no represented pixels and sizes on both sides of one pixel per tile.
The new test and three existing minimap/topology tests passed on macOS arm64;
the client and test targets built successfully. Build and test logs on TB5:
`runtime/minimap-radar-build-2026-11-15.log` and
`runtime/minimap-radar-tests-2026-11-15.log`. The build emitted existing
duplicate-library and missing Conan search-path linker warnings.

The test checks the logical rectangle, not its dashed rasterization, physical
screen scaling, GUI appearance, or continuous camera motion. It also does
not establish pixel representation for every map tile when the map is larger
than the display.

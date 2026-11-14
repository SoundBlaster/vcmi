# Movement on the minimum toroidal map

The branch accepts toroidal maps at least three tiles wide and high.
`HeroMovementTest.MinimumToroidalMapSupportsEveryEdgeAndCornerOnBothLevels`
uses a revealed, empty 3×3 SoD map with two levels. For each of eight
directions on each level, test setup relocates the hero to an appropriate
edge or corner. The pathfinder finds a direct wrapped neighbor, and a real
server movement handler reaches that tile with the same movement points the
pathfinder predicted. The level is preserved.

The new test, the previous 3×3 corner case, and the larger-map road direction
test passed three consecutive runs on macOS arm64. The setup relocations are
not player actions. This checks clear terrain and movement only; multi-tile
object footprints, vehicles, save/load, AI and graphical rendering on 3×3
maps remain unverified.

`HeroMovementTest.MinimumToroidalMapCollectsResourcesAcrossEverySeam` adds a
gold resource at each wrapped destination in turn. Across all eight directions
and both levels, pathfinding chooses the adjacent resource; the server charges
the predicted movement, credits 1,000 gold and removes the resource while the
hero remains at the source. A second request moves the hero onto the cleared
tile. All three minimum-size tests passed three repeated runs. These are
synthetic single-resource cases, not a general test of overlapping footprints
or other object types.

`HeroMovementTest.MinimumToroidalMapGuardCoversWrappedNeighborsWithoutCrossingLevels`
places one hostile monster on the surface. The guard cache identifies it for
each surface cell on the 3×3 torus and reports no guard on the underground
level. A server movement request attacking through the opposite corner starts
a battle with that monster at its canonical position. All four minimum-size
tests passed three repeated runs. This does not test multiple guards, battle
resolution, or guard visibility through fog on such a small map.

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
not player actions. This checks clear terrain and movement only; object
footprints, guards, vehicles, save/load, AI and graphical rendering on 3×3
maps remain unverified.

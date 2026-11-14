# Impassable terrain across edges and corners

`HeroMovementTest.ImpassableTerrainBlocksEveryToroidalEdgeAndCorner` checks
eight outward directions on each of two levels of a 36×36 toroidal map. For
each, the destination is temporarily made rock after the synthetic map starts.
The actual pathfinder reports no reachable LAND node, and the server rejects
the direct move without changing the hero's position or movement points.
Restoring the original terrain then allows the same move as a positive control.

The new test and five existing seam and obstacle tests, including RoE and SoD,
passed three consecutive runs on macOS arm64. The TB5 log is
`runtime/block-seam-directions-test-2026-11-14.log`. This does not test every
object mask, partially hidden terrain, graphical input or remote clients.

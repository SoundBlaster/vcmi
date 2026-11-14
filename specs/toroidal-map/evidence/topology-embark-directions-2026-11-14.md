# Embarking and disembarking across seams

`HeroMovementTest.EmbarkAndDisembarkAcrossEveryToroidalSeamOnBothLevels`
uses a synthetic 36×36 toroidal map with two levels. For each outward
cardinal and diagonal direction on each level, it places the hero on a clear
shore and a boat in the adjacent wrapped water cell. The real pathfinder
must choose the direct step, and the server must accept the embarkation and
the reverse disembarkation with the same remaining movement points. The
test removes each boat before setting up the next case.

The test passed three consecutive runs on macOS arm64. This establishes
16 clear-shore configurations and their reverse transitions in the local
test harness. It does not establish behavior around other terrain, guards or
objects, multiplayer, or graphical animation.

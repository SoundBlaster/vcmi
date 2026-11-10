# AI patrol radius at map seams

The shared pathfinder initializes a computer-controlled hero's patrol area
through `getTilesInRange` with the Manhattan distance formula. For toroidal
maps that callback enumerates wrapped coordinates on the hero's level;
bounded maps keep their original clipped enumeration.

`HeroMovementTest.aiPatrolRadiusUsesWrappedManhattanDistance` starts a
computer-controlled hero at `(0,0,0)` on an open, revealed 36×36 SoD map and
sets its patrol radius to two. It calculates actual paths once with bounded
topology and once with wrapping enabled. Both variants can reach `(1,0,0)`
and `(1,1,0)` but not `(2,1,0)`. Only the toroidal variant can reach
`(35,0,0)`, `(0,35,0)` and `(35,35,0)`; neither can reach `(34,35,0)`.

The test passed on macOS arm64 using the Release build at `5009fa0` plus
the test change. This is a shared-pathfinder test, not a live Nullkiller2
decision or an exhaustive audit of AI behavior, other levels or obstacles.
The build exited successfully with existing duplicate-library and missing
Conan search-path warnings.

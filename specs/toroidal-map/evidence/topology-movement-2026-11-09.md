# Toroidal movement: build and test attempt

On 2026-11-09 the pathfinder, server movement check, map adjacency, guard
lookup and sight-range calculation were adapted to the selected topology.
Direction updates use the short wrapped displacement. This is an implementation
slice: it does not establish that the assembled game, every interaction,
client presentation, boat travel, AI, multiplayer or saves conform.

Baseline: commit `2251673` plus the then-uncommitted changes identified below.
The Release CMake build of `vcmitest`, `vcmiclient`, `vcmiserver`, and
`vcmieditor` completed with exit 0 on macOS arm64. It used the TB5 build root
recorded in `topology-unit-2026-11-09.md`; the log is `movement-build.log`
under that root. Linker warnings about duplicate system libraries and absent
Conan system-library search paths remain.

From `cmake/bin` under the TB5 build root, the following test attempt exited 1
before either test body, during the global game library initialization:

```sh
./vcmitest --gtest_filter='HeroMovementTest.pathfinderAndServerCrossToroidalSeams:HeroMovementTest.sightRangeWrapsWithoutRevealingDistantTiles'
```

Diagnostic: `Resource with name DATA/LCDESC and type TEXT wasn't found.` The
original game resources are not available locally. Both tests were compiled;
neither gameplay result was observed.

```text
a83bb5af214742230cfadb4a2bfa8558cdbd7c08413c13126206b9672597dc7e  lib/pathfinder/CPathfinder.cpp
3dfeb5473e924395465414307ab19cbfa5b69128167846c27d66a0ad80bdf202  server/CGameHandler.cpp
8b88cc4be9567a3dd33320c56105b715158cf9494797363d79bb807e7bef806b  lib/mapping/CMap.cpp
fb8e3b7863b4d758797b52453638144446dbb0991ba68fce9c014308f1a747c8  lib/callback/CGameInfoCallback.cpp
73d8b0691cc7717b2de28d5865fc7b2a324c2aa2397b73823791ead9ce01e83b  lib/gameState/CGameState.cpp
ee15c0e9e79dadc79f20e0b534a9b8ebbefd119aa7762b93d8dad60c34b62a08  lib/gameState/GameStatePackVisitor.cpp
fe64ce3d8cc0b7183d641a15d29db5d6337754d068829b19afb2b76a75c1713d  test/game/HeroMovementTest.cpp
```

# Save file after crossing a seam

On 2026-11-09 on macOS arm64 with copied Complete assets, a SoD game-state
test performed a server-authorized step from (0,1) to (35,1), then wrote and
read a complete `.vsgm1` file. The test checks the topology in the saved
header and reloaded state, the hero position, one revealed tile and one
hidden tile. The temporary file was written inside the TB5 build directory
and removed by the test.

`vcmitest` built successfully (`save-seam-file-build.log`); from `cmake/bin`,
`./vcmitest --gtest_filter=HeroMovementTest.crossedSeamAndFogSurviveGameStateReload:SaveGameTest.*`
passed six tests (`save-seam-file-tests.log`). This does not verify a GUI
save/load, all fog cells, multiplayer, or other seam directions.

Source baseline `1e0c81e` with local test change:

```text
d205ab3de64cd52b2050075a848f5b7f85532dcbe8add72240dfac827ab6d7b5  test/game/HeroMovementTest.cpp
```

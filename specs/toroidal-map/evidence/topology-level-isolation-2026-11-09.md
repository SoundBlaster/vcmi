# Underground seam and level isolation

On 2026-11-09 on macOS arm64 with copied Complete data, a two-level SoD
fixture moved a hero from (0,1,1) to (35,1,1) through the underground seam.
The server kept the hero on level 1, revealed the destination there, and
left the corresponding level-0 tile hidden. It rejected a direct ordinary
move from level 1 to level 0. After revealing both levels, the pathfinder
found the wrapped underground neighbor directly and found no route to the
surface on this fixture without passages.

`vcmitest` built (`level-seam-build.log`); from `cmake/bin`,
`./vcmitest --gtest_filter=HeroMovementTest.*:SaveGameTest.*` passed 20 tests
(`level-seam-suite.log`). Normal passages between levels, corner crossings,
other profiles, and GUI presentation remain unverified by this test.

Source baseline `060ac3b` with local test change:

```text
bbfb555b9ef8681d593f5bccbbb43078b03ef6135573a79210e6d51fc80fb592  test/game/HeroMovementTest.cpp
```

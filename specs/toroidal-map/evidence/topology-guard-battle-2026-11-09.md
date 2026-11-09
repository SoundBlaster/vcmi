# Battle with a guard across a seam

On 2026-11-09 on macOS arm64 with copied Complete data, an SoD fixture
placed a hero at (0,7) and a hostile monster at (35,5). Moving the hero to
(0,6) entered the monster's wrapped guard range. The server started a battle
whose defending army is the monster on the other side of the seam.

`vcmitest` built (`guard-battle-build.log`); from `cmake/bin`,
`./vcmitest --gtest_filter=HeroMovementTest.*` passed 17 tests
(`guard-battle-tests.log`). Combat completion, guard variants, corner guards,
and GUI battle presentation are not established by this check.

Source baseline `c9c7756` with local test change:

```text
c3c1e705b569352162ba9c83afcc8f0bc760e9260bd797f3710aaf21fccf715a  test/game/HeroMovementTest.cpp
```

# Battle with a guard across a seam

On 2026-11-10 a server regression test at source revision `7ba620b` showed
that a flying-capable hero could request `LAND` transit and bypass a guard
across the corner. The test failed before the fix. On toroidal maps the
server now applies the flying/water-walking guard bypass only to the
corresponding requested movement layers; bounded-map behavior is unchanged.
The regression and a real `AIR` transit case both passed in the 40-test
`HeroMovementTest` suite. The full suite passed 1,385 tests with three
disabled (`full-suite-transit-guard.log`). This does not resolve the separately documented
absence of an authoritative source movement layer. TB5 build output and
test logs use the `transit-guard-before-*` and `transit-guard-after-*` names.

On 2026-11-10 at source revision `177f97c` plus a new test, removing a
monster at `(35,35,0)` through the server refreshed the cached guard position
at `(0,0,0)`, `(0,35,0)` and `(35,0,0)`. The new test and the existing
guard-position and corner-battle tests passed on macOS arm64 with Complete
data. The TB5 build root contains `corner-guard-refresh-build.log` and
`corner-guard-refresh-test.log`. This checks object removal, not combat
completion or multiple overlapping guards.

On 2026-11-10 a synthetic SoD scenario additionally placed the guard at
`(35,35,0)` and moved the hero from `(0,1,0)` into `(0,0,0)`. The server
started a battle with that guard as defender. The new corner test, the earlier
edge battle test and guard-position test all passed on macOS arm64 (three
tests); `vcmitest` was built from `d94dd32` plus the test
(`corner-guard-battle-build.log` on TB5). Combat animation, victory and all
guard configurations remain unverified.

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

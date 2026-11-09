# Boat movement across a seam

On 2026-11-09 on macOS arm64 with copied Complete data, an SoD fixture
tested a hero embarking across the horizontal boundary, sailing back across
it, and disembarking across it. Before each server movement, the pathfinder
found a same-turn route, and remaining movement points matched the server's
result. The test changes the two tiles' terrain between steps to isolate the
three transitions, so it is not a playthrough of a fixed authored water map.

`vcmitest` built (`boat-seam-build.log`); from `cmake/bin`,
`./vcmitest --gtest_filter=HeroMovementTest.*:MovementCostTest.*` passed 16
tests (`boat-seam-suite.log`). Boat animation, other directions and corners,
blocked shores, AI and multiplayer remain unverified.

Source baseline `985bb2c` with local test change:

```text
fc385d4e44ccafac4db998805aae18e1683a9ef85a59fd07596c6792ef0ffd41  test/game/HeroMovementTest.cpp
```

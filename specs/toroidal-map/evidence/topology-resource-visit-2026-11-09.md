# Resource interaction across a corner

On 2026-11-09 on macOS arm64 with copied Complete data, an SoD fixture placed
a hero at (0,0) and a gold pile at (35,35). A server-authorized diagonal
action collected 1,000 gold across the corner and removed the resource while
the hero remained on the original tile, as required for this blocking visit.
A second action moved the hero onto the vacated tile without giving the
resource a second time.

`vcmitest` built (`object-interaction-build.log`); from `cmake/bin`,
`./vcmitest --gtest_filter=HeroMovementTest.HeroCollectsResourceAcrossToroidalCorner`
passed (`object-interaction-tests.log`). This covers one resource interaction,
not guarded resources, other objects, client input, or multiplayer.

Source baseline `8d04656` with local test change:

```text
466342e4654c5d1883eef56a14c628a2eef672ae9f1cf4c151e4bcefd904a063  test/game/HeroMovementTest.cpp
```

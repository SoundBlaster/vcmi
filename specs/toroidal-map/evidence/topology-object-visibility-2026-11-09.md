# Visibility of wrapped object footprints

Object visibility now checks the canonical position of each visible footprint
tile. It preserves the existing rule that at least one covered tile must be
visible and keeps bounded-map clipping. The static seam-footprint restriction
remains in force until remaining object interactions are addressed.

On 2026-11-09 on macOS arm64 with copied Complete assets, `vcmitest`,
`vcmiclient` and `vcmieditor` built (`object-visibility-build.log`). The
full `./vcmitest` run passed 1,329 tests with three disabled
(`object-visibility-full-tests.log`). A new game-state test constructs a
2×2 visible footprint at (0,0) with only the opposite corner (35,35)
revealed: it is hidden on a bounded map, visible on a torus, and hidden
again when that cell is concealed. This does not test GUI fog rendering,
other profiles, or the full set of object interactions.

Source baseline `766c331` with local changes:

```text
610e3a05f61032cac4cf54e20bdb95365aa3499de393c4257c7b0281c39f925b  lib/mapObjects/CGObjectInstance.cpp
ed4c3b57c14eabad42357683fa07b0b90ff63f90ea80b2b3c8949e0a29b0f4fb  test/game/HeroMovementTest.cpp
```

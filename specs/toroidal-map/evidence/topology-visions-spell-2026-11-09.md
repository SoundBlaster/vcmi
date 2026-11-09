# Visions spell range across a seam

The Visions bonus now measures distance through the shortest map-topology
displacement on toroidal maps, while retaining the existing bounded-map
calculation and same-level requirement. Coordinates are canonicalized before
the displacement is calculated; this also accommodates an object anchor
outside the canonical rectangle. This applies to this bonus range check, not
to every spell or information disclosure path.

On 2026-11-09, macOS arm64, a new `HeroMovementTest` failed before the fix
for an object across the west seam and one across the northwest corner
(`visions-before-tests.log`). After the change, the new test also checked
bounded near and far targets, a distant toroidal target, another map level,
and the absence of a Visions bonus. The Release `vcmitest` build succeeded
(`visions-after-build.log`); `HeroMovementTest.*:MapTopology.*` passed 25 tests
with copied Complete data (`visions-after-tests.log`). This is a game-state
test, not a GUI spell-casting test.

Source baseline `e0466e9` with local changes:

```text
b8bea1f14a2b1b769b5086cfca7526ddec97ba7daf8db8103cb5567dde783691  lib/mapObjects/CGHeroInstance.cpp
00f4e56ae732c57d3086c6a41b7377215ed7fbddb24da2da3acdba8f3e071c9e  test/game/HeroMovementTest.cpp
```

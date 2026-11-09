# Toroidal terrain brush

On 2026-11-09, the terrain editing operation was changed to use canonical
neighbors at toroidal seams when expanding a brush selection, validating
terrain patterns, and invalidating terrain views. Bounded maps keep bounded
neighbors. This applies to terrain editing, not road or river editing, and
does not automatically repair existing maps when topology is enabled.

On macOS arm64 with copied Complete assets, `vcmitest` and `vcmieditor` built
successfully (`terrain-seam-build.log`). From `cmake/bin`,
`./vcmitest --gtest_filter=MapManager.*` passed three tests
(`terrain-seam-tests.log`). The new test compares terrain types in a 7×7
neighborhood and transition orientation in a 3×3 neighborhood after painting
one grass tile at a toroidal corner versus the interior of a 20×20 water map.
It also checks undo/redo for one wrapped tile. The existing bounded terrain
type and view tests passed. Sprite frame variants, other brush shapes, both
levels, road and river continuity, and GUI editing are not proven by this run.
The complete local `./vcmitest` run passed 1,324 tests with three disabled
(`terrain-full-tests.log`).

Source baseline `11020e4` with the following local changes:

```text
71771a8647feeba4b87e8e7e105f579842915f516b61dddcc87b03cf3a151076  lib/mapping/CMapOperation.cpp
ddcf22458a691cf0a292fdb581cf507edd4240a566b12343b4a719a8bbaa5e0f  lib/mapping/CMapOperation.h
d3831261ca1a22294e291ca7846c0eee81ed0d0f33f0c030e85e2e9828af2a18  test/map/CMapEditManagerTest.cpp
```

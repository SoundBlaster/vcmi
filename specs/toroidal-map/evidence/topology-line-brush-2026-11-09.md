# Toroidal road and river brushes

The line-editing operation now invalidates and matches neighbors across the
topology boundary. On bounded maps, out-of-map neighbors remain absent.
This covers the road and river editing algorithm; enabling topology does not
automatically redraw lines already present on the map.

On 2026-11-09 on macOS arm64 with copied Complete assets, `vcmitest` and
`vcmieditor` built successfully (`line-seam-build.log`). From `cmake/bin`,
`./vcmitest --gtest_filter=MapManager.*` passed four tests
(`line-seam-tests.log`). The added test draws a three-tile turning road and
river across each axis on a 20×20 toroidal map. It checks the selected corner
image group and verifies recalculation after undo and redo. Other line shapes,
GUI editing, existing maps and rendered texture continuity remain unverified.
The full local `./vcmitest` run passed 1,325 tests, with three disabled
(`line-full-tests.log`).

Source baseline `edc93b5` with local changes:

```text
89a3a26727f554b52a94af441f49a6e2f97e5e2bb8d2236b80cf73bb761f991b  lib/mapping/CDrawRoadsOperation.cpp
186f2bccba608548ca5c4a5e18eeb7b18c5ed340a79ca69923acf1cd0e960c52  test/map/CMapEditManagerTest.cpp
```

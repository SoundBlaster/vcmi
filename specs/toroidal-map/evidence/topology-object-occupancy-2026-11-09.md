# Toroidal object occupancy groundwork

Map occupancy updates now map each blocking or visitable footprint tile to
its canonical location on toroidal maps. Mask tests still use the original
relative tile. Removal clears the same locations. Bounded maps keep their
existing clipping. The implementation avoids duplicate object entries if a
footprint is larger than a map period.

This is groundwork only: the existing map authoring and loading restriction
for static seam-spanning objects remains in force. Draw order, object-local
coordinate queries, visit locations, visibility and interactions have not
been established for those objects.

On 2026-11-09, macOS arm64, `vcmitest`, `vcmiclient` and `vcmieditor` built
successfully (`footprint-seam-build.log`). With copied Complete data,
`./vcmitest --gtest_filter=MapObjectDrawOrderTest.*:HeroMovementTest.*`
passed 20 tests (`footprint-seam-tests.log`), including an artificial 2×2
blocking/visitable footprint crossing both axes, its removal, and bounded
clipping. `./vcmitest` passed 1,326 tests with three disabled
(`footprint-full-tests.log`). After adding an oversized-footprint test, the
`MapObjectDrawOrderTest.*` suite passed nine tests
(`footprint-oversized-tests.log`); the full run preceded that test addition.

Source baseline `5c3fff3` with local changes:

```text
1cdc97f989b8e8ca42fd6eed6718e41cf62da29ebb9511098300bcebe6bf2a92  lib/mapping/CMap.cpp
be9d665e688cbc16cc1ea8adcda6a3c79d6da3b523a4ab713fa345237330bc0b  test/map/MapObjectDrawOrderTest.cpp
```

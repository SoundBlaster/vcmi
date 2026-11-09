# Toroidal object draw-order groundwork

The shared draw-order calculation now resolves object-local layer coordinates
across toroidal seams, including comparisons on overlapping tiles. Both the
client and editor pass the map to this calculation. This does not yet stamp
or render static seam-spanning objects in either UI; the map restriction
remains in force, and footprints larger than the map period are not supported
by this ordering proof.

On 2026-11-09 on macOS arm64 with copied Complete assets, `vcmitest`,
`vcmiclient` and `vcmieditor` built successfully (`draworder-seam-build.log`).
From `cmake/bin`, `./vcmitest --gtest_filter=MapObjectDrawOrderTest.*:HeroMovementTest.*`
passed 22 tests (`draworder-seam-tests.log`). The new test compares ordering
of a tree crown and a rock across the north/south seam in both insertion
orders with the previously tested interior behavior. The full `./vcmitest`
run passed 1,328 tests with three disabled (`draworder-full-tests.log`).
Corner overlap, real sprites and GUI rendering are unverified.

Source baseline `287a773` with local changes:

```text
1e48b8107170b4740a76e8836656fa12296c9c3aafe728a05c9ca178fcfabf6b  lib/mapObjects/MapObjectDrawOrder.cpp
814e11bbd4041d7b6f81e3e566e37cd8b6c44ee6da1d8cfd137f26715bbfaf76  lib/mapObjects/MapObjectDrawOrder.h
4bab53568893e8ac337b171bbf373cf16d123ef592a325d21f378ac11871201c  client/mapView/MapRenderer.cpp
7358293e9a84ae3c675f785123c9209007783d51b6ab1f16fc9cf027cdc9d3c5  mapeditor/maphandler.cpp
8429da3d59498aa4472817518662244b6ab75a3a732696a78750d370395253a0  test/map/MapObjectDrawOrderTest.cpp
```

# Switching topology in the editor

Changing the editor's topology setting now rebuilds indexed blocking and
visitable cells before recalculating guards and refreshing the editor views.
The rebuild starts from objects already present in the occupancy index, so
objects intentionally hidden from the map remain hidden. It preserves the
relative order of objects already sharing a tile. Objects with no indexed
blocking or visitable cells do not need an occupancy entry.

On 2026-11-09 on macOS arm64 with copied Complete assets, `vcmitest` and
`vcmieditor` built (`topology-switch-build.log`). Focused map, movement and
draw-order suites passed 28 tests (`topology-switch-tests.log`). The added
test switches a map on and off, checks a wrapped footprint, preserves a
hidden object, and checks the order of two overlapping shown objects. The
complete `./vcmitest` run passed 1,330 tests with three disabled
(`topology-switch-full-tests.log`). This does not establish behavior for all
object types, save/reload after a GUI edit, or static seam interactions.

In the rebuilt editor GUI, a copied toroidal demo map was opened and
`Map → General → General → Connect opposite map edges` was switched off,
applied, reopened to confirm the off value, then switched on and applied.
Terrain and objects remained visible and the UI responded after each change.
The editor closed without saving; the source and copy retained SHA-256
`c08c08bd577654b31415855efa3a6eccaf3bf4de65485a767741d3ffdf48d4e3`.
The observation was made through UI automation; no screenshot was saved.
It does not verify persistence or movement through the seam.

Source baseline `768833d` with local changes:

```text
227576158c09e8b7f95af13afe8c7500750c1b316e16731e508328f45f03c46e  lib/mapping/CMap.cpp
552bdd4abf430152b133718a10bcb05b7899a17124f85c377047b1fda664333f  lib/mapping/CMap.h
551a95bbad03bdf0e5503eac55eab173b07ba6437071763460f1e65dc2d8b4be  mapeditor/mapsettings/generalsettings.cpp
4aa82c5ceb7c9ea9ed90d0365137e164f9dbe0960341f6ef31fe2d2fcf72f364  test/map/MapObjectDrawOrderTest.cpp
```

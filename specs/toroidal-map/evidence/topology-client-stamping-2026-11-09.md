# Client object stamping groundwork

The client now indexes static object footprint tiles by canonical map
coordinates and selects the corresponding unwrapped image slice. The shared
object-coordinate helper is also used for draw ordering. This is groundwork:
static seam-spanning objects remain rejected by map loading and authoring,
and editor stamping and object interactions are not covered by this change.

On 2026-11-09, macOS arm64 with copied Complete assets, `vcmitest`,
`vcmiclient` and `vcmieditor` built (`client-stamp-build.log`). The focused
`MapObjectDrawOrderTest.*:HeroMovementTest.*` selection passed 22 tests
(`client-stamp-tests.log`), including object-local coordinate checks. The
full `./vcmitest` run passed 1,328 tests with three disabled
(`client-stamp-full-tests.log`). These tests do not exercise client drawing.

A spectator GUI regression with the rebuilt client and the synthetic demo
map showed terrain and objects and camera movement west and north. The
minimap radar split on both axes. The frame retained outside the repository at
`runtime/evidence/gui-toroidal/regression-64a84a4-north-west.png` has SHA-256
`9f26462817738b410ac7dde6e084fa3c4211b20a97e42a378a3bf1b53d989970`.
The original art has visible terrain discontinuities, the map contains no
seam-spanning static objects, and the single screenshot cannot prove smooth
animation or player fog. The test client required forced termination.

Source baseline `64a84a4` with local changes:

```text
503679871fc702ead241edb663620dedeb6dc7a60bfae93e1cfaabdf2ebf272c  lib/mapObjects/MapObjectDrawOrder.cpp
da961dcc2e63191a04aa300fcb9ff78c93466ecb9904f7837fcd3aae715f45ea  lib/mapObjects/MapObjectDrawOrder.h
d0ac71b8b2c12fd36ab5d2bce9191fb67da04b1f1c10fbcf27dba2039aac775c  client/mapView/MapRendererContextState.cpp
d2a02794efdfaa16ae39a34a658fd715983c8f53dd5adc7fa48af44d30d914ef  client/mapView/MapRendererContext.cpp
0b0d07cc4b8d3417cb5beb027b6d5ef02692d6be33e0716380bca5a7fd89f557  test/map/MapObjectDrawOrderTest.cpp
```

# Toroidal viewport: build record

On 2026-11-09 the adventure camera, render tile mapping, fog neighbor access,
mouse input, route arrows, hero/boat sprite stamping and minimap radar received
an initial toroidal implementation. The change is incomplete: static objects
whose drawn footprint spans a seam are not yet handled, and none of these
changes has been observed in a running game. Build success is not visual
continuity, input, animation or multiplayer evidence.

Source baseline: `6389865` plus the then-uncommitted source files below.
The macOS arm64 Release CMake build of `vcmitest`, `vcmiclient`, `vcmiserver`
and `vcmieditor` completed with exit 0 using the TB5 build root recorded in
`topology-unit-2026-11-09.md`; `viewport-build.log` is under that root.
Linker warnings about duplicate libraries and absent Conan system-library
search paths remain. VCMI gameplay was not launched: the original game data
needed by the engine is absent.

```text
71429b72969b8494ea7967a7771b646c8af9336b544b0c26d8be5fea8473f54c  client/mapView/MapViewController.cpp
63608cbc9a3c5ce8ff02b150d83056bf4794403824d5e8c8983df8256c4c9c19  client/mapView/MapViewActions.cpp
c53abe5db78ce4be8696e241b82ab3a22f8c05139ca2a593b9e1f098ef2a93a9  client/mapView/MapRenderer.cpp
f686401678a87014b8a33c5e05f62be699295a6d60e5d4e7a3dee5e73e643d35  client/mapView/MapRendererContext.cpp
a7b89544f064e3b42dd793e9bd8e34e96a78a4a41474af521fafddb2bf54b323  client/mapView/MapRendererContextState.cpp
1b8d2deb995c1ab7fa2bc9ef6f1331610dcc522c363db0a0f892a8ce88de4957  client/adventureMap/CMinimap.cpp
```

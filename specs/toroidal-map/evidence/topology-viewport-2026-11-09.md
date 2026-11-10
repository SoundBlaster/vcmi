# Toroidal viewport: build record

## Camera geometry check, 2026-11-10

On a 3×3 torus, a sprite as wide as a map period can cover the same canonical
tile with two image copies while moving; at a corner, four fragments may be
needed. The previous renderer used only one image offset for that tile. The
toroidal renderer now enumerates overlapping periodic copies and clips each
source rectangle and destination position to the tile. A geometry test checks
every pixel at five movement samples across all four corners for a 96×96
image on a 96×96 period, plus one partially visible smaller image. The
standalone topology suite passed 14 tests and `vcmiclient` built on macOS
arm64 (`wrapped-sprite-fragments-final-build.log` and
`wrapped-sprite-fragments-final-test.log` on TB5). These checks establish
fragment geometry; game frames and performance have not been measured.

A later SDL3 software test drew a synthetic 96×96 image through VCMI's
`Canvas` and `SDLImageShared` into a 32×32 surface using the same fragment
enumeration. The resulting RGBA values matched the expected periodic image
for every pixel at five movement samples across all four corners. The separate
CTest target `WrappedSpriteSdl` passed on macOS arm64
(`wrapped-sprite-sdl-build.log` and `wrapped-sprite-sdl-test.log` on TB5).
It uses a minimal `IImage` adapter at scale 1; it does not exercise the full
map renderer, actual game sprites, alpha/color effects, the GPU backend or
live animation.

A second test subsequently sent the same fragments through `Canvas`'s
texture path and `SDLImageShared::drawTexture`, then read back every pixel
from an SDL software renderer. It passed for the midpoint of one diagonal
crossing (`wrapped-sprite-texture-build.log` and
`wrapped-sprite-texture-test.log` on TB5). This exercises the texture API
with a software renderer, not a hardware GPU or a running game.

On 2026-11-10, source `5d572ca` plus the texture-test change expanded that
check to every integer pixel position in a 32-pixel step, for all four edge
directions and all four corner directions (264 samples). Each sample clears
the target first and checks every RGBA pixel against the expected periodic
synthetic image. The `vcmi-wrapped-sprite-sdl-test` target built and CTest
`WrappedSpriteSdl` passed on macOS arm64, with the CMake tree on TB5. The
linker reported duplicate libraries and missing Conan search-path warnings.
This tests an SDL software renderer through the texture API, at scale 1;
it does not establish full map composition, real artwork, a hardware GPU,
presented frames, frame timing or perceived scrolling smoothness.

```sh
cmake --build <TB5-build>/cmake --target vcmi-wrapped-sprite-sdl-test -j 4
ctest --test-dir <TB5-build>/cmake -R '^WrappedSpriteSdl$' --output-on-failure
```

At revision `406969f` plus the movement-offset extraction, the same sprite
offset calculation used by the adventure renderer was checked for all eight
seam directions, including all four corners. Five interpolation positions
from start to finish stayed within one tile; a bounded-map check retained
ordinary displacement. A model-level following-camera check independently
reconstructed the normalized camera position and verified a stable sprite
screen position at those samples. Both `vcmiclient` and the asset-independent
test target built, and all 13 standalone tests passed
(`movement-offset-build.log`, `movement-offset-test.log` on TB5). This is
calculation evidence, not proof of real frame timing, camera-controller
integration, actual image content or the boat animation.

At source revision `6a442e2` plus the cache-layout change, the actual band
enumeration used by software blitting and GPU presentation was moved to
`MapViewModel` without changing its algorithm. The standalone test checks
that each tile in the view is represented exactly once, with matching cache
and screen positions, at all four normalized seam/corner camera positions,
two viewport sizes, two zoom levels and both cache scaling modes. The larger
viewport spans multiple map periods. `vcmiclient` and the standalone test
target built; all 12 standalone tests passed (`cache-band-build.log` and
`cache-band-test.log` on TB5). This verifies rectangle geometry only; actual
texture contents, scaling artifacts and consecutive rendered frames remain
unverified.

At source revision `0adab6f` plus the new standalone test,
`MapViewModelTopologyTest.WrappedCameraPreservesPixelGeometryAcrossSeams`
passed for one-pixel horizontal, vertical and diagonal camera moves across
the seam, including a reverse diagonal move. It compares canonical tile
coordinates and screen rectangles at sampled pixels for a 36×36 map and two
viewport sizes, one larger than the map period. All 11 standalone topology
tests passed without game assets. Build and test output are in the TB5 build
root as `view-model-topology-build.log` and `view-model-topology-test.log`.

This is a geometry check on `MapViewModel`, not an observation of controller
normalization, rendered frames, sprite animation or interactive scrolling.
Later game observations are recorded separately in the GUI evidence files.

## Initial build, 2026-11-09

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

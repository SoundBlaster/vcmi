# Hero icons at toroidal minimap edges

Previously the client drew each visible hero icon once, clipped to the minimap.
When the icon extended beyond a toroidal edge, its clipped pixels did not
appear on the opposite edge. The client now draws periodic copies for the
toroidal minimap, including both axes at corners. Bounded maps keep the
previous single draw. The existing visible-hero filtering remains in place.

`MapTopology.ToroidalMinimapIconCopiesCoverEdges` compares coverage with an
independent per-pixel modulo model for axis extents 1–20 and icon lengths
1–16, including images larger than an axis. The SDL3 software-canvas test
`WrappedSpriteSdl.ToroidalMinimapHeroIconWrapsAcrossEdgesAndCorners` checks
the colored pixels of a 7×8 synthetic icon at all four corners and an interior
position. Both passed; two existing SDL3 minimap raster tests passed as well.

On macOS arm64, the client and both test executables built with the change.
The full local `vcmitest` run passed 1,927 tests, skipped the external legacy
server test, and reported three disabled tests. The log and XML are under
`runtime/full-suite-minimap-icons-2026-11-15.*` on the checkout's TB5 build
root. The linker emitted duplicate-library and missing Conan search-path
warnings; the build succeeded.

This is software-canvas and geometry evidence. It does not establish actual
GUI appearance, fog interactions, other rendering backends, or scrolling and
animation continuity. The tests use synthetic icon pixels and do not compare
the original game artwork.

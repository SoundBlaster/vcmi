# Artifact pickup across seams

On macOS arm64, `EdgesAndBoundedControls/ToroidalArtifactPickupTest.*` passed
32 cases in the TB5 CMake build. The binary was built with base revision
`fd36898` and the test subsequently committed as `76af62c`.

The parameterized test creates an unguarded artifact of type 7, stages a hero
at the source and executes a server move to the artifact. It checks successful
movement, possession of that artifact and removal of the map object. It covers
all eight directions at edges and corners on each of two levels on toroidal
maps, plus the same directions and levels inside bounded maps. It does not
test guarded artifacts, client path selection, GUI dialogs or multiplayer.

The XML result is local to TB5 at `runtime/artifact-seam-final-2026-11-15.xml`,
SHA-256 `911dd281598e10c336ffac8775a8fb112c2c5cd065c16f7cf8b76c9ddf3e8376`.

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

## Guarded artifact

A subsequent build based on `ea89bd2`, with the test committed as `dd6261a`,
passed all 64 cases in the same suite: the 32 unguarded cases and 32 guarded
cases. Each guarded case places one creature on the artifact, refuses the
dialog and verifies that the object and visit state remain usable, then
revisits and accepts the battle. The server test helper forces victory; after
the battle result is accepted, the hero holds the artifact, the object is
removed and the visit has finished. The guarded cases use the same eight
directions, two levels and bounded controls as the unguarded cases.

The second XML result is local to TB5 at
`runtime/guarded-artifact-final-2026-11-15.xml`, SHA-256
`367ce4bcd2cea8815a70fe916bc2ddf92759302497046dc76a1e9ea832aebb1e`.
Retreat, defeat, graphical dialogs and remote clients are not covered.

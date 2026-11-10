# Toroidal boat GUI fixture: 2026-11-10

## Scope and fixture

The test `MapFormat.ToroidalBoatDemonstrationFixtureRoundTrips` generates a
36×36 single-level SoD VMAP with toroidal topology, a shore at `(0,0,0)` and
a boat at `(35,0,0)`. It reloads the VMAP and checks the water, shore and
boat. Optional output is only written after assertions pass and refuses an
existing file. The actual generated map in the isolated TB5 game profile has
SHA-256 `7aaecb4c356ec86101aaff420998a30810b17587df4537ab6cb5ed8c0d0904dc`.
The first attempted one-tile island was converted to water by terrain redraw;
its invalid output was retained separately and was not used for this run.

## GUI observation

A signed macOS client built from VCMI revision `c2457bd` loaded the generated
map. The hero embarked west across the seam from `(0,0,0)` to `(35,0,0)`;
the diagnostic selected-hero observation changed from 1560 to zero movement.
On the next turn, directional input sailed through the four cardinal seam
crossings: `(35,0)` → `(35,35)` → `(0,35)` → `(35,35)` → `(35,0)`.
Movement decreased from 1500 to 1100 in steps of 100. Finally the hero
went east across the seam onto land at `(0,0)`, leaving the boat; movement
became zero. The GUI showed the mounted hero and the empty boat after landing.
Observation records 74–98 in the local player JSONL cover the run; records
79–96 cover sailing and records 97–98 the landing.

A separate screen recording captured the sailing, not embarkation or landing.
The requested 35-second capture produced 12.918 seconds; the cause was not
established. The 3600×2338 movie has 735 frames, average 56.83 fps, SHA-256
`67949e13cc8c4f4c2e262a1db2c3800912f10b6002010c12967657e3bab443c4`, and is
stored at `/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/boat-seams-c2457bd-2026-11-10.mov`.
The boat remained visible in the inspected 1 fps contact sheet and 25 fps
resampled crops around the four movements, without an obvious jump in those
samples. This does not verify every original frame, frame pacing, diagonal
sailing, other boats or shores, or a client built from the newer source.

## Automated verification

On the TB5 CMake build, `vcmitest --gtest_filter=` selected
`MapFormat.ToroidalBoatDemonstrationFixtureRoundTrips`,
`MapFormat.AdaptedToroidalDemonstrationFixture`,
`HeroMovementTest.BoatCanSailAcrossToroidalCornerAndBothAxes`, and
`HeroMovementTest.HeroCanEmbarkAndDisembarkAcrossToroidalCorner`.
All four passed. The GUI and automated checks do not establish full support
for every map, game mode or animation frame.

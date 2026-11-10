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

## Current-build GUI follow-up

A signed bundle containing the client, server and shared library rebuilt from
clean VCMI source revision `afb8044` was launched on the same Mac. It loaded
the same VMAP; its window title included `1.8.0.afb8044`. The bundled client
and library SHA-256 values after dependency-path rewriting and signing were
`3045aa03d55f5dd040cdfb7643d10acc94d332fce4eb4ecb159d897baec7d6b8` and
`5061917785c77703f7956f89051f19a9727894af363aaa1896f6dad4948eb77e`.
The bundle passed `codesign --verify --deep --strict`. The client and server
build completed with duplicate-library and missing-search-path linker warnings.

On the real game window, the hero embarked `(0,0)` → `(35,0)`, then on the
next day sailed `(35,0)` → `(35,35)` → `(0,35)` → `(35,34)` → `(0,35)` →
`(35,35)` → `(35,0)` and disembarked at `(0,0)`. Thus the selected-hero state
and sampled GUI showed crossings in both directions on both axes and a diagonal
crossing of the horizontal seam in each direction. Each cardinal move cost 100
movement points and each diagonal move cost 141. The test also tried selecting
`(0,35)` from `(35,0)` diagonally across both seams: a longer path was shown
and no move executed by the directional shortcut. This fixture has land at
`(0,0)`, adjacent to that diagonal; it is not evidence that a fully open-water
corner crossing fails or succeeds in the GUI.

The immutable local copy of observation records 0–26 is
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/boat-gui-afb8044-observations-0-26.jsonl`,
SHA-256 `c3693b4e1794dd09d9176ea7c1b212284c3727598bc252a818f5b582314176ac`.
The follow-up used still GUI observations, not a continuous recording, and does
not independently establish frame pacing or the smoothness of diagonal movement.

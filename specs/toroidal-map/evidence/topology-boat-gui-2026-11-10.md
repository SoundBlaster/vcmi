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

## Open-water diagonal corner follow-up

`MapFormat.ToroidalOpenWaterCornerFixtureRoundTrips` generates and reloads a
36×36 toroidal VMAP with water at all four corners, a red hero at `(30,30)`
and a boat at `(31,30)`. The test verifies every intended water tile and the
boat after the round trip; optional output refuses to overwrite a file. The
fixture used below has SHA-256
`2524eccd4396bc76d8d8d8f3509b15ad01c50249bb8128d41c7cb92444911e79`.

The signed client, server and shared library were built from revision
`3478269`; only the fixture test was uncommitted. On the real game window,
the hero embarked `(30,30)` → `(31,30)`. On day two it sailed via `(32,31)`,
`(33,32)`, `(34,33)`, `(35,34)` to `(35,35)`, then crossed both seams
diagonally `(35,35)` → `(0,0)` → `(35,35)`, crossed east to `(0,35)`, and
sailed diagonally `(0,35)` → `(35,0)` → `(0,35)`. Selected-hero observations
show each diagonal consumed 141 movement points and the eastward step 100.
The final GUI still displayed the boat. These observations establish the
transitions for this boat and fixture; they do not verify other shoreline
configurations, boats, AI, or continuous animation.

The local player-state records 0–36 were copied to
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/boat-corners-3478269-observations-0-36.jsonl`,
SHA-256 `9b3e8179b88ec5598214dfaf4ef11b9e492428e1b26bc020504fe791baad1092`.
A simultaneous screen-recording attempt produced no saved movie; the Mac's
internal disk was nearly full during the attempt. No causal diagnosis or
claim about movement smoothness follows from these still observations.

## Current-client diagonal recording

On 2026-11-14, a signed macOS arm64 client built from `8552be9` with the
scenario-list event-coordinate fix later committed as `a56a719` loaded the
same open-water fixture. The hero embarked `(30,30) → (31,30)` and, on the
next day, sailed to `(35,35)`. Diagnostic shortcuts then produced
`(35,35) → (0,0) → (35,35) → (0,35) → (35,0) → (0,35)`; the four diagonal
steps cost 141 points each and the eastward step cost 100. The extracted
observation records 48–86 are stored locally under the VCMI TB5 build root at
`runtime/boat-corner-8552be9-observations-48-86.jsonl`, SHA-256
`ac760ba7262771907279391618a3b7d2eebf256c29218c13c85316efebd228b2`.

A separate 20.008-second screen recording during those commands has 1,119
video frames at 3600×2338, approximately 55.9 fps average. It is stored at
`runtime/boat-corner-fd44e5f-2026-11-14.mov` in the same TB5 build root,
SHA-256 `e9d079edd54549c2ae444e35827622448d00d5df74a175e6d5a3cbf11b99c768`.
The file name is a local label, not the client's Git version. A 1 fps sheet of
the whole recording and four one-second crops resampled at 25 fps were
inspected. The boat remains visible and changes direction in those samples.
A system notification covered part of the viewport throughout; the camera
follows the boat and the water has few stable landmarks. The recording includes
the surrounding desktop and is not committed. These samples do not establish
uninterrupted animation, exact synchronization between the recording and
observation clocks, frame pacing, or continuity across the obscured region.

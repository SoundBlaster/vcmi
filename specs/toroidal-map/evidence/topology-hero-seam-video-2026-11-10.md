# Hero animation across both seams

On macOS arm64, the development-signed VCMI client built from `c2457bd` ran
the isolated 36×36 Toroid Seam Laboratory save in fullscreen. After the first
day ended, four arrow-key actions were issued with approximately 1.5 seconds
between them. A 35-second screen recording captured the game, without an audio
capture option. The recording was made independently of the sparse Screener
checkpoints and stored on TB5, not committed with the game assets.

The player's version 2 diagnostic trace records the selected hero moving
`(0,35) → (35,35) → (0,35) → (0,0) → (0,35)` on level 0. During those
crossings the camera's canonical pixel center passes `1151 → 1136`,
`1137 → 9 → 16`, `1138 → 9 → 16`, and `15 → 1144 → 1136`, respectively.
These values are sampled observations, not every displayed frame. The existing
trace file was appended to, with the relevant observations at sequences 54–68.

The movie is 3600×2338, 35.002 seconds and 1,998 video frames (about 57.1 fps
average). A 1 fps contact sheet of the complete movie and four 25 fps resampled
contact sheets, cropped around the hero, were visually inspected. In these
sampled frames the hero stays present, changes direction and moves relative to
the nearby ore and wood; no obvious abrupt jump was observed for these four
steps. The selected video segment around 4–11 seconds contained 400 decoded
frames; consecutive presentation timestamps averaged 17.52 ms, with a maximum
of 41.67 ms and no interval above 50 ms. These are recording timestamps, not
an independent measurement of the game's render or display cadence.

The recording is
`runtime/hero-seams-retry-c2457bd-2026-11-10.mov` in the VCMI TB5 build root,
SHA-256 `c9c5606950d27a8bf5587114f501adb5bb9a21399fc39715477c3d6df06ca043`.
The contact sheets and appended observation file are beside it. An earlier
15-second trial finished before movement and is not evidence for animation.

## Simultaneous corner crossing

A separate 25.008-second screen recording of the same client and game captured
one southwest diagonal move using the End key. The source observation records
sequences 69–71: hero `(0,35,0) → (35,0,0)`, 141 movement points spent, and
camera center `(1149,3) → (1136,16)`. Both axes therefore crossed their map
period in one move. A 25 fps resampled contact sheet around the transition
shows intermediate images with the hero and nearby resources still present;
no obvious jump was observed in this cropped sample. An attempted reverse
action used an unsupported Computer Use key name and was not performed.

The second movie is `runtime/hero-corner-c2457bd-2026-11-10.mov` in the same
TB5 build root, 3600×2338, 1,438 frames, approximately 57.5 fps average;
SHA-256 `2bc054164662ecaaae4f344d410f8405f6b8b86e51cc08595ceb90bc0009aa1d`.

These checks cover only the inspected samples for one hero, terrain, camera
mode, zoom and local client. Other diagonal directions, other artwork, boats,
full-frame-by-frame inspection, visual latency and current builds after
`c2457bd` remain unverified.

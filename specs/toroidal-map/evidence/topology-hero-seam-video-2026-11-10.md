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

This verifies only the inspected samples for one hero, terrain, camera mode,
zoom and local client. The four directions cross one axis at a time; diagonal
corner animation, other artwork, boats, full-frame-by-frame inspection, visual
latency and current builds after `c2457bd` remain unverified.

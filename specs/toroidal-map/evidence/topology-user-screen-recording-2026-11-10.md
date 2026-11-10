# User screen recording of camera movement

On macOS arm64 the user recorded the signed `c2457bd` diagnostic client on
the 36×36 `Toroid Seam Laboratory` saved game. The user controlled the
camera after the game was loaded at the corner. The recording is the user's
local desktop MOV (SHA-256
`d6fce61cd8bbf28ddf601646fcc8c866adeb1a51cddfac2fde4bdc74d1318909`),
not a repository artifact. `ffprobe` reports 8.96 seconds, 3600×2338 H.264,
514 encoded video frames and a variable average frame rate of approximately
57 fps. Recording timestamps are not a measurement of game render latency.

The same client session's opt-in trace recorded 35 change-driven snapshots.
For a 1,152×1,152 pixel map period, its horizontal camera center changed from
`(1094, 1136)` to `(23, 1136)` and its vertical center later changed from
`(23, 63)` to `(23, 1128)`. The previous and following samples are about
100 ms apart. The matching visual movement order is visible in the recording;
the trace is not synchronized to individual video frames. An isolated copy
of the session trace is stored in the checkout's TB5 runtime directory as
`user-recording-session.jsonl` (SHA-256
`3c750d0c1170249e50939b261054f2d6f6c19b4505e94420e10254bd9bc13fb1`).

A contact sheet sampled the clip twice per second; a further two-second
segment was sampled at eight frames per second and a half-second horizontal
segment was resampled at 60 frames per second. In these inspected samples,
the revealed terrain and town moved together without an apparent seam jump;
their repeated copies entered or left the viewport as the camera moved. The
full clip was not individually inspected frame by frame. The recording does
not establish frame pacing under all conditions, a simultaneous diagonal
corner crossing, moving hero or boat animation, roads and rivers, or other
map artwork. The original file was not altered or added to the repository.

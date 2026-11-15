# Earth demo in an isolated macOS profile

Commit `e63938e` adds an opt-in `VCMI_USER_DATA_PATH` override on macOS.
It must be an absolute path and relocates user data, configuration, cache,
saves and logs; an unset or empty value retains the original paths. The
environment is inherited by processes launched by the client. The unit test
checks the related paths, empty-value fallback, and rejection of a relative
path. `vcmiclient --version` also reported all of those paths under the
selected directory. This is an experimental branch feature, not an upstream
VCMI capability.

The complete `vcmitest` run in a separate TB5 user-data directory passed
2,004 tests, skipped eight optional external scenarios and listed three
disabled tests. The output and XML are `isolated-profile-suite.log` and
`isolated-profile-suite.xml` under the Earth research runtime. The test
profile used local links to the existing base game data; it was not the
playable Earth profile.

The local profile is `runtime/earth-demo-profile/` under the TB5 build root.
It contains its own mod settings, game settings, saves and logs. Its `Data`,
`Mp3`, HotA mod and map entries point to locally available resources; it does
not redistribute game assets or the third-party map. The local executable
`Start Earth Demo.command` sets the profile path and launches the branch
client. The named TB5 volume and the existing build and resource paths must
remain available. The launcher was run directly, reached the main menu and
exited with code 0 after the test requested and confirmed exit.

The tested client was built from `e84d72b` plus the source changes committed
as `e63938e`. In a separate spectator launch from this profile, the
graphical client emitted a toroidal map observation of 252×576 and Nullkiller
began acting. Then a normal human game was started from the main menu. The
scenario list displayed `Земля володій світом !!! 2.0`, size 252×576, when
filtered by `2.0`. The red player was human and the other seven used
Nullkiller. A diagnostic eastward key command moved the selected hero from
`(164,353,0)` to `(165,353,0)` and reduced movement points from 1,560 to
1,510. The test requested application exit, confirmed the in-game prompt,
and the client reported a clean shutdown with exit code 0.

The profile's `logs/earth-filter.png`, `logs/earth-setup.png` and
`logs/manual-check.jsonl` contain the selected scenario, initial game view
and state observations. Their SHA-256 values are
`c337a1c0d03929a889dae9a2a75a18b1e7566429515d8dd4aff2cb66ae7c5fd9`,
`4316259a7809790689ffddf4941e83600d3576a944304ae955c5321982c47fef` and
`9e0db934318b8a9e9cac4502a25246c1b427dcb8651a15d2181e5f5560ee492e`,
respectively. The original user profile's mod settings retained SHA-256
`9b75c7b29e43efb1946c65bc40c84cd28f01f3d36aa557d90db28f9190e1bb43`.

These observations establish a selectable, locally startable draft and one
human movement, not seamless animation, seam appearance, balanced gameplay,
complete HotA compatibility, multiplayer or a full playthrough. The client
also reported a libpng interlace warning and legacy HotA map warnings while
scanning other scenarios. Further visual acceptance remains with the user.

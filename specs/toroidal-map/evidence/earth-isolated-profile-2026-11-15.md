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

## Updated local build and launcher working directory

The existing local launcher previously invoked the client by absolute path
without changing the working directory. The build reports its game data and
server paths as `.` and `./vcmiserver`, so invoking the script from another
directory could resolve those relative paths incorrectly. The local launcher
now changes into the TB5 CMake `bin` directory before executing
`./vcmiclient --nointro`; its isolated `VCMI_USER_DATA_PATH` is unchanged.
This is a local launcher adjustment, not an installed application or a
tracked portable launcher.

The client and server were rebuilt from clean revision `109a663` on macOS
arm64. Both `--version` commands reported that revision and the Earth profile
paths. Invoking the corrected launcher with `/Users/egor` as its initial
working directory loaded core and HotA content, initialized the Metal renderer,
loaded main-menu resources and started menu music. The unbundled process did
not appear in the available Computer Use application inventory; the test was
terminated after checking the log, without interacting with the GUI. This
does not verify Finder double-click, game selection, actual play, or graceful
exit in this updated build. The earlier graphical evidence remains tied to
its named revisions.

The map symlink still resolves to the assembled Earth draft, SHA-256
`97f5222ab48e5987b58c12643be985e450524488bf042a9f6ae27ab9f03ea1f3`.
The updated client, server and library SHA-256 values are respectively
`a359a0fc4ce81309db52721ac4e0c2c9c5273ab8660ee71fb2e1ca64de6002b7`,
`f9300f92324d266f65f8f77ca5c7216d50555837c748fad7049de451ad4cb0c9`, and
`f317f1911f96f0724a794eeb46bdc02303debd64ee3602e0a97bc1efc237c29c`.
The build reported duplicate-library and missing Conan search-path warnings.

A subsequent launch of the same binary and profile created its diagnostic
socket in a private directory. A desktop screenshot showed a window titled
`VCMI - Open Heroes 3 1.8.0.109a663`, partly covered by another active window;
the game itself was not operated. Computer Use could not select the unbundled
executable as an application. The test process was forcibly stopped without
starting a game. This confirms that a window was created, not that the Earth
scenario was selected or that the launcher works through Finder.

## Integrated road map: current AI startup

The client and separate server executable were rebuilt on TB5 from clean
revision `7744dbd` on macOS arm64. A new isolated profile linked the exact
integrated road map, SHA-256
`61f9f042612fb86aa611c64e57b2c2e6ee7a39409cf2b4eeb1fd74b8b00d40b7`,
and copied the Earth profile configuration while linking to the same local
game resources. With `--testmap`, seed 17 and a spectator interface, the
client observation reported a 252×576 toroidal map. During the first red AI
turn, the log recorded town construction, hero recruitment and movement
requests with server and client processing. The run used an in-process local
server and was force-stopped after 25 seconds; it does not establish normal
exit, a full turn or match, movement across the seam, human play or visual
continuity. The new profile did not use the ordinary Earth's saves.

The logs and observation are local on TB5 at
`runtime/earth-integrated-smoke-7744dbd/`; `stdout.log` SHA-256 is
`97c699430a7036b975453f90059bff3a2f09629cdb90bf37d02c5dc2aad8595a`
and `observe.jsonl` SHA-256 is
`6a39ba56bb4875a8b604f43b1a9c5102f31c167687837b8f9e043f116f0b996d`.
The build retained duplicate-library and missing Conan search-path warnings.

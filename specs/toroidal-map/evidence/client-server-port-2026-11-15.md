# Client server-port override

The client parsed `--serverport` into session settings, but server runners and
the multiplayer menu read the configured ports instead. The session override
now takes precedence for local hosting, the displayed local port and the
default remote port. It is not written into persistent settings. Values from
1 to 65535 are accepted; zero retains the configured value. Negative values
and values above 65535 are rejected before game initialization.

On macOS arm64, the client and server built from `9852983` plus this change.
The build emitted the existing duplicate-library and missing Conan search-path
warnings. CLI probes for `--serverport=-1` and `--serverport=65536` exited 1
with the expected error. A signed graphical client started with
`--serverport 3033` displayed that port in the multiplayer menu. A hosted
TCP lobby listened on `*:3033`, confirmed with `lsof`. The opt-in
`GameConnectionTopologyTest.ConnectsToExternalCurrentServer` connected on
loopback, negotiated the current toroidal-capable protocol and passed; the
host log recorded the probe joining and leaving. This checks a lobby handshake,
not a complete multiplayer game.

The host was closed without starting a game. Its listener disappeared and the
SHA-256 of the user's `settings.json` was unchanged across the test. The
`GameConnectionTopologyTest.*` suite passed 15 tests, with two optional
external-server tests skipped when their environment was not configured.
The test client used the in-process server; the separate process runner,
configured-port fallback, remote join UI and full game over this port were
not exercised in this run.

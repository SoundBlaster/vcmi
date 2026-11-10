# Map topology format: build and test attempt

## Tagged VMAP writer

New toroidal VMAP headers use integer major version `0x54520004` and retain
`topology: "toroidal"`. The loader accepts that marker and the branch's old
version-4 prototypes; a different marker, a tagged header with invalid
topology, and a toroidal value in a legacy version are rejected. Bounded maps
continue to use version 3. Before the writer change, two tests failed because
the generated header contained untagged `4`. An intermediate implementation
using a JSON float rounded the marker during writing; storing it as a JSON
integer fixed the round trip. Eight focused map and demo tests passed, with one
optional unrelated save test skipped. Client, server, editor and test targets
built on macOS arm64. A further regression demonstrated that a 64-bit unknown
version could truncate to the marker before validation; the loader now checks
the original value and rejects that case and negative versions. Logs
`vmap-marker-before-test.log`, `vmap-overflow-before-test.log`,
`vmap-marker-validated-build.log`, and `vmap-marker-validated-test.log` are
on TB5.
The full `vcmitest` run passed 1,415 tests, skipped the optional external-save
test and retained four disabled tests (`full-suite-vmap-marker.log` on TB5).
`specpm validate . --json` reported no errors or warnings.

The original version-3 source rejects a larger major version, but no old
binary was run. Existing untagged version-4 prototypes are still ambiguous,
and this marker is not allocated by upstream. A later optional test read the
previously authored `toroid-seam-laboratory.vmap` from the isolated TB5
runtime, confirmed its untagged version 4, serialized it with the new marker
in memory and reopened it with the same dimensions and object count. It did
not start gameplay or compare every object property. The source file's SHA-256
remained `3fbc0a2cbbdfa22068e7e6a099a7b51e803d39b5fc0d0b3fd95155cc6eeb232d`
before and after the test. Set `VCMI_TOROID_PROTOTYPE_VMAP` to repeat this
optional test; `vmap-prototype-test.log` records this run on TB5.

On 2026-11-10 the format test at revision `8384c5d` showed that a version-3
header with an unknown string or boolean `topology` was silently loaded as a
bounded map. Both new assertions failed before the fix. The loader now
rejects an explicit unrecognized or malformed topology; a missing field and
an explicit `bounded` field retain bounded behavior, and `toroidal` still
requires the version-4 format. The focused map-format and save-header tests
passed, and client, server, editor and test targets built
(`unknown-topology-before-test.log`, `unknown-topology-after-build.log`,
`unknown-topology-after-test.log` on TB5).
The full local suite passed 1,395 tests with three disabled
(`full-suite-unknown-topology.log`).

The lobby compatibility question is separate: `LobbyClientConnected` sends a
numeric serialization version, not a toroidal capability identifier, and the
server start guard checks that version. A future unrelated upstream version
could reuse this branch's number. Existing packet framing rejects unread
bytes, but neither that check nor this loader validation proved all such
peers safely distinguished at that revision.

An in-memory `GameConnection` test subsequently serialized the real
`LobbyUpdateMapInfo` packet. With both codecs set to the current version the
toroidal flag survives; with both set to the preceding version the flag is
omitted. A deliberately mismatched codec experiment rejected the older wire
layout when read as current, but the reverse direction accepted the current
layout without an error while reading the map header as bounded. This is a
simulation using two versions of the current binary, not a test of an actual
old or future peer. It demonstrates why the numeric-version collision cannot
be dismissed as a framing error. The expected fail-closed regression is
recorded as disabled until explicit capability negotiation exists. TB5 logs:
`lobby-packet-final-build.log`, `lobby-packet-final-test.log`, and the
intentionally failing `lobby-packet-conflict-test.log`.
The normal full suite passed 1,397 tests with four disabled
(`full-suite-lobby-packet.log`); the fourth disabled test is this newly
recorded unresolved safety regression.

The local branch now implements the experimental tagged greeting described
in [network capability design](../network-capability-design.md). A tagged
peer is negotiated at the normal serialization version; an untagged peer
with the same ordinal is capped to the previous layout and excluded by the
server's toroidal start guard. An untagged server at or beyond the branch's
toroidal ordinal is rejected by the local client as ambiguous; this also
restricts bounded games against such unverified future servers. Earlier
untagged versions remain eligible for bounded sessions. The tag is not an
upstream allocation or authentication, and map/save version collisions
remain unresolved.

The current `GameConnection` tests round-trip the tagged greeting, invoke
the real server greeting visitor and topology guard with two tagged and two
untagged simulated connections, and check legacy and ambiguous greetings.
Six focused tests passed; the client, server and test targets built. The full
suite passed 1,401 tests with four disabled (`lobby-capability-final-build.log`,
`lobby-capability-final-test.log`, `full-suite-lobby-capability-final.log` on
TB5). These are in-process packet and server-method tests, not a real TCP or
old-binary multiplayer session. The raw-codec mismatch regression remains
disabled as defense in depth: bypassing the handshake can still misinterpret
the packet.

A subsequent test at revision `8f81ff1` used VCMI's real internal connection
and network event loops to connect a tagged and an untagged simulated client
to `CVCMIServer`. It received both lobby acknowledgments and checked the
server's topology guard: the tagged peer alone was eligible; after the
untagged peer joined, a toroidal map was rejected while a bounded map remained
eligible. The individual test and seven-test focused suite passed
(`lobby-internal-build.log`, `lobby-internal-test.log`,
`lobby-internal-suite.log` on TB5). This does not start a full game or run
actual old or separate-process client binaries.

The same negotiation and guard-method check was subsequently exercised over
TCP on `127.0.0.1`, with the real server and two probe clients in one process.
The server accepted a tagged peer, downgraded an untagged peer, rejected the
toroidal header for the mixed set, and still allowed a bounded header. The
focused suite passed eight tests with one disabled. This is a loopback transport
check, not a game start or a multiplayer session between actual clients.

A separate test generates a VMAP from the adapted demo map and calls the real
server's `prepareToStartGame()`. Game-state initialization succeeds for a
compatible connection and is rejected for an incompatible one. The connection
capability is set directly in this test; it does not combine the TCP handshake
with the game start or call `startGameImmediately()`. The complete local suite
passed 1,404 tests with four disabled (`full-suite-server-start.log` on TB5).

On 2026-11-09 a versioned topology flag was added to the map header, JSON map
header and binary serialization. A version-4 map header signals `toroidal`;
legacy maps and older save formats default to bounded. The server checks a
connected client's negotiated serialization version before starting a
toroidal session. Width and height below 3 are rejected for this topology.
The format version is local to this branch and has not been accepted upstream.

Read-only comparison with the original checkout baseline
`30e9207276f137658da5132705c05644bfe33aee` shows that its VMAP loader sets
`VERSION_MAJOR` to 3 and throws when a map header has a larger major version.
Thus its source is designed to reject this branch's version-4 VMAP rather
than silently load it as bounded. This is source inspection, not execution
of an old binary, and does not resolve the possibility of a later upstream
release assigning version 4 a different meaning.

Source baseline: `1ad28cd` plus the then-uncommitted changes identified below.
The Release CMake build on macOS arm64 completed with exit 0 for `vcmitest`,
`vcmiclient`, `vcmiserver` and `vcmieditor`, using the same checkout-specific
TB5 build root and toolchain as the topology helper record. Log:
`format-build.log` under that root. The linker repeated its warnings about
duplicate system libraries and absent Conan system-library search paths.

Attempted from the build's `cmake/bin` directory:

```sh
./vcmitest --gtest_filter='MapFormat.ToroidalTopologyRoundTripsAndRequiresNewFormat:SaveGameTest.mapTopologyIsVersionedInSavedHeader'
```

The process exited 1 during the global test environment setup, before either
test body ran: `Resource with name DATA/LCDESC and type TEXT wasn't found.`
Original Heroes III resources are absent. The two test sources were compiled,
but the format round-trip, legacy save behavior and network compatibility are
**not verified** by execution. The standalone coordinate tests reported in
the separate unit record do not cover these claims.

On 2026-11-10, with the copied Complete resources, a Release `vcmitest` build
from `3c8e2e1` plus the added header test passed
`MapFormat.ToroidalTopologyRoundTripsAndRequiresNewFormat` and
`MapFormat.ToroidalHeaderRequiresAtLeastThreeTilesOnEachAxis`. The second test
loads headers and checks that 3×3 is accepted, while 2×3 and 3×2 are rejected;
it does not initialize or play a complete 3×3 map. The command ran from
`cmake/bin` with those two names in `--gtest_filter`; the TB5 log is
`toroidal-header-minimum-tests.log`. This later run supersedes the earlier
failed attempt only for the named round-trip test, not network compatibility.

A subsequent `vcmitest` build from `6f0cac1` plus a new gameplay test passed
`HeroMovementTest.HeroCrossesMinimumToroidalMapCorner` alongside the minimum
header test. The synthetic 3×3 SoD map was initialized and wrapping enabled;
the pathfinder selected the direct `(0,0,0) -> (2,2,0)` step, which the server
accepted with matching movement points. This is one server-side movement,
not a complete 3×3 map loaded from a toroidal VMAP or a client rendering test.
The TB5 build log is `toroidal-minimum-game-build.log`.

SHA-256 of selected changed files at the time of the build:

```text
299aa0dcc522c254a7a831d7aa7bb5547539a05b229352ae5d1cd8ec8616e52b  lib/mapping/CMapHeader.h
6769aeb4861f1a84bbf820acced4d3736e933c866ac9297137b0b8ac805728a2  lib/mapping/MapFormatJson.cpp
2823a0f79bb4cd3abc81c0de60d26f0b22e51058dd4acda8c3fa57bb7bcc0d74  lib/serializer/ESerializationVersion.h
2e8fefaf2fb58af3cee136f7b125ddbe62c1f3808232fea23376188db7dd95e6  lib/serializer/GameConnection.cpp
ccc7334f2d1bd82b51e9bde43682fd2766c4a2141df7030bb92a8eba3204985c  server/CVCMIServer.cpp
da1ca8009702981f0b7a8d8463564929e23c23e12fae8b700d09dceb6d395edc  test/map/CMapFormatTest.cpp
b97488caa4721e1ce0eb062337a97755f1669e0d84873ae09fbadbad08006865  test/game/SaveGameTest.cpp
```

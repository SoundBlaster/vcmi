# Map topology format: build and test attempt

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
bytes, but neither that check nor this loader validation proves all such
peers are safely distinguished. Explicit capability negotiation and upstream
version allocation remain unresolved.

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

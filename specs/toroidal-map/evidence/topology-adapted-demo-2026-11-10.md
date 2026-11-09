# Adapted toroidal demonstration map

`test/mock/ToroidalDemoMap.h` defines a deliberately sparse 36×36 grass map
with two players, two towns, and resources positioned to exercise both seams
and one corner. It is an experimental mechanics fixture, not a balanced game
scenario. The first hero starts at visitable tile `(0,0,0)`; the corner resource
is at `(35,35,0)`. The fixture does not depend on redistributing game assets.

`MapFormat.AdaptedToroidalDemonstrationFixture` loads the generated H3M,
explicitly enables wrapping, checks the current static-footprint restriction,
saves a toroidal VMAP and reloads it. It checks both heroes and towns, the
three intended resource cells, and that no tile has multiple visitable objects.
The same fixture can write the VMAP to a **new**, explicitly named path by
setting `VCMI_TOROID_DEMO_OUTPUT`; normal test runs write no such file.

On 2026-11-10 on macOS arm64 the local generated VMAP was
`runtime/user-data/Maps/toroid-seam-laboratory.vmap` on TB5, SHA-256
`3fbc0a2cbbdfa22068e7e6a099a7b51e803d39b5fc0d0b3fd95155cc6eeb232d`.
The output path resolves to the isolated copied Complete-data tree, not the
user's original installation.

`ToroidalDemoGameTest.HeroCrossesCornerAndBothEdges` initializes a game state
from the same H3M fixture with wrapping enabled. Through the real server move
handler it collects the resource across the corner, enters the vacated tile,
then crosses the horizontal and vertical seams back to the start. Two focused
tests passed (`adapted-demo-game-final-tests.log`); the test target built
(`adapted-demo-game-final-build.log`). The gameplay test uses the fixture
in-memory, not a game session loaded from the saved VMAP.

The later `ToroidalDemoVmapTest.SavedMapStartsAndSupportsCornerVisit` creates
and serializes the same fixture, loads the resulting in-memory VMAP through
`CMapService` into a fully initialized `CGameState`, and uses the real server
move handler to collect the corner resource. Together with the earlier two
fixture tests it passed three tests (`adapted-demo-vmap-tests.log`). This run
loads an independently generated in-memory archive, not the exact file above;
it is not a client UI or visual rendering test.
The same VMAP test was extended to run `CPathfinder` before the server move;
the corner resource is reachable with the start tile as its immediate
predecessor. All three fixture tests passed (`adapted-demo-route-tests.log`).
The VMAP test was then extended to enter the vacated corner, write a temporary
`.vsgm1` save file on TB5, reload its game state, verify the hero position,
topology and gold, and continue across the horizontal seam. Four focused
tests passed (`adapted-demo-save-tests.log`). This does not verify the GUI
save/load flow or all fog-of-war state.

An ordinary ad-hoc signed client was prepared on TB5 from the current build;
the signed executable SHA-256 was
`369c4e941468769afff789f10e69644855646bfda6e99ff621da1bab5a2469d4`.
On launch macOS requested removable-volume access for
`local.vcmi.toroid.lab`; the user was busy and no response to that specific
prompt was observed. No GUI playthrough, visual seamlessness, map balance,
AI behavior on this fixture or multiplayer result is claimed.

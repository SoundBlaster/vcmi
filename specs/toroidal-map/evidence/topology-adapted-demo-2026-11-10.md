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
`local.vcmi.toroid.lab`; the user later authorized the game launch and the
client reached the menu. The window reported `1.8.0.a2bf933`, an older build
stamp; this observation does not establish the exact shared libraries mapped
in the running process or a visual test of the latest minimap change.

Using Computer Use keyboard input and screenshots, the tester selected the
on-disk **Toroid Seam Laboratory** in the ordinary scenario list and started
it. At the starting corner, keypad 7 collected the diagonally adjacent gold:
the GUI displayed a 1,000-gold pickup and the total rose from 20,000 to
21,000. A second keypad 7 entered the vacated corner; keypad 6 and keypad 2
then returned across the horizontal and vertical seams. The minimap marker
appeared successively at the bottom-right, bottom-left, and top-left corners.
The integrated server log records successful hero-anchor moves
`(1,0,0) -> (36,35,0) -> (1,35,0) -> (1,0,0)`; the corresponding visitable
tiles are `(0,0,0) -> (35,35,0) -> (0,35,0) -> (0,0,0)`. The resource visit
before the first move left the hero in place. The trace is in
`runtime/user-logs/VCMI_Client_log.txt` on TB5.

The same client saved the session through the game interface after the later
three-step movement. The confirmation reported success, and the save picker
showed the new save under the laboratory scenario. Loading it restored the
visible 21,000 gold and the hero. Four keypad 7 moves then took the hero back
to the starting corner and across it: the minimap marker appeared at the
bottom-right corner, and the integrated server trace recorded the successful
anchor move `(1,0,0) -> (36,35,0)`. This tests one GUI save and reload with
continued corner movement; it does not establish preservation of all game
state or frame-by-frame visual continuity.

These screenshots are discrete observations; they do not establish
frame-by-frame scrolling or animation smoothness. Initially mouse input did
not activate the expected controls while the older test client was also open.
After the older client exited, a click with the pointer displayed over a
resource instead displayed a route to another tile; a following click started
the route, and another selected the town. The trace records the resulting
three-step movement to `(3,3,0)`. This is an unresolved discrepancy in this
Computer Use setup: the observation alone does not distinguish input delivery,
coordinate scaling, focus, or a game defect. It is not a successful mouse
selection or minimap-selection test for this fixture. Map balance, AI behavior
on this fixture and multiplayer remain unverified.

After the save/reload check, another click on an ordinary town portrait did
not select it. As a diagnostic the tester switched from fullscreen to windowed
mode using F4 and tried the town portrait again; Computer Use returned
`windowNotFoundAtPosition` before delivering that click. F4 restored the
fullscreen setting. This adds an input-tool/window-location limitation to the
observations; neither the source of the original discrepancy nor ordinary
physical-mouse behavior has been established.

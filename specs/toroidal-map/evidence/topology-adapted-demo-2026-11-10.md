# Adapted toroidal demonstration map

## Later client launch attempt

After the observations below, a freshly copied and ad-hoc signed lab client
was launched. It remained running for more than 50 minutes without reaching
an observable game window or producing a new client log. A macOS `sample`
showed its main thread in `dyld` while opening a dependency; `lsof` showed
the executable and `dyld` but no loaded game libraries. Computer Use listed
the process as running. These observations do not establish which file was
being opened or whether an OS permission prompt caused the stall. This
attempt does not verify the newer source changes in a running client.

A later targeted macOS unified-log query found a removable-volume access
request for this exact process at launch. TCC reported that the previously
recorded code requirement did not match the newly signed application and
recorded `AUTHREQ_PROMPTING`; the filtered log for that request contained no
response through the time of inspection. Together with the `dyld` sample,
this is evidence consistent with the launch waiting on that access request,
but it does not independently show the prompt on screen or prove the cause
of the blocked `open`. Filtered local logs are `tcc-lab-launch.log` and
`tcc-lab-request.log` in the TB5 build root.

After the user reported allowing the launch, the stalled process was stopped
through Activity Monitor and the same lab bundle was launched again through
Computer Use. The new process also remained in `dyld` before opening a game
window; attaching Computer Use to it timed out. A targeted TCC log query for
the new process showed an All Files preflight but no new removable-volume
prompt or response. The reason for this second stall is therefore unconfirmed.
This retry did not verify the current source revision in the GUI.

The separate `VCMI Toroid Test` bundle, which had reached a game window in an
earlier session, was also launched through Computer Use. Its launch timed out
without an observable window; Activity Monitor showed a one-thread process.
The targeted TCC log showed an All Files preflight but no removable-volume
prompt for that attempt. The test process was closed through Activity Monitor.
This observation does not establish that the two launch failures share a cause.

Later inspection through System Settings showed the Removable Volumes switch
enabled for both `VCMI Toroid Lab.app` and `VCMI Toroid Test.app`. No privacy
setting was changed. This is the setting currently displayed by macOS; it does
not resolve the earlier logged code-requirement mismatch or establish why the
new processes stalled before opening their game windows.

The stale client was stopped before preparing the current build because its
old executable referenced the former `CSaveFile` constructor while the build
directory contained the new library. A backup of that bundle was retained on
TB5. The updated lab bundle copies the build's client, server and editor, and
keeps `Mods`, `config` and `scripts` under `Contents/Resources/Data` with
relative links inside the bundle. The old bundle failed strict code-signature
verification because its links pointed outside the bundle; the new ad-hoc
signed bundle passed `codesign --verify --deep --strict`. The client executable
SHA-256 was `c8be9ffa72138be00ce963d7026230d33b2c86c22bb1abe8eefedd2d00a75a68`;
the linked build library SHA-256 was
`fd65024087b8f56fe2b8116e932e09e18f6700b02c26d870373958f955455bbb`.

Launching that bundle through Computer Use timed out without a game window.
A new process remained in `dyld` opening a dependency; the targeted TCC log
showed an All Files preflight for it but no removable-volume request or
response. The strict-signature correction alone therefore did not produce a
working launch. The file being opened and the cause of the wait remain
unknown; no visual behavior from this new client has been established.

As a separate diagnostic, the same strict-verified 33 MB bundle was copied
temporarily to the internal disk and launched there; the linked build
libraries and the isolated game data remained on TB5. This process also
stalled in `dyld` during an `open` before displaying a window. The sample did
not reveal the path being opened, so it cannot establish which volume caused
the wait. The process was stopped and the temporary internal copy removed;
the TB5 bundle and its earlier backup remain.

## Initial fixture and GUI observations

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

## Later client startup diagnostics

A rebuilt diagnostic app was tested after the save-format changes. With the
client and its dependencies outside the app bundle, dyld stopped after finding
`libvcmi.dylib` on TB5. Moving that library into the signed bundle moved the
last dyld diagnostic to `libminizip.dylib`. A further test bundled 14 direct
and transitive libraries; dyld loaded them and the process entered `main`.
Process samples then showed its main thread waiting in `open` from
`CBasicLogConfigurator::configureDefault`, before a game window appeared.
The normal log path resolves through the isolated test log symlink to TB5.
The samples do not include the system call's path argument, so the precise
file and the operating-system cause are not independently established.

Setting a temporary `HOME` in the test app's `LSEnvironment` did not change
the running process's `HOME`; a second sample showed the same logging stack.
The existing Lab and Test entries displayed removable-volume access as enabled
in System Settings; the new Diagnostic app did not appear in that list. A
system log recorded an All Files preflight for Diagnostic, not a confirmed
removable-volume denial. These observations do not prove that permissions are
the cause, nor do they verify the latest client visually. No system privacy
setting was changed. Diagnostic dyld logs were retained in the isolated TB5
runtime; the stalled processes were closed after inspection.

A subsequent diagnostic launcher set `HOME` before executing the same client.
With the logs in a temporary local directory and the test data still on TB5,
the client log recorded initialization through the SDL3 backend. A sample
then showed `CResourceHandler::initialize` waiting in `open` while constructing
a recursive directory iterator. The first loaders in this function use the
test save and configuration directories; the sample does not expose the
actual path argument. No game window appeared. This shows progress beyond
the earlier logging wait, but does not determine the cause of the new wait.

After the later Dimension Door and Scuttle Boat tests were added, the full
`vcmitest` run passed 1,415 tests on macOS arm64. One optional prototype-save
test was skipped without its environment variable and four tests remained
disabled; its separate earlier run is recorded in the save-marker evidence.
The full-run log is `full-suite-post-spells.log` on TB5.

The next diagnostic used an isolated temporary profile with copied settings,
an empty saves directory and links to the game data on TB5. It progressed
beyond the save/config loaders, then stopped in `CResourceHandler::createInitial`
while opening a directory. A trace added before each filesystem scan identified
the last requested path as the profile's `Data` link, which resolves to the
isolated `complete-data/Data` directory on TB5. The last launch used a client
stamped `309a6f0` and the traced library, and did not reach a game window.
The system log recorded an All Files preflight and response, but no observed
removable-volume denial; the operating-system cause remains unconfirmed.
The launch log is retained as `runtime/vcmi-diagnostic-traced-data.log` on TB5.

While adding the trace, the loader's error message was found to use a moved-from
path argument. A symlink-loop test reproduced the missing path in the message;
the message now uses the stored path. Five filesystem/resource-path tests
passed after the fix (`filesystem-path-before-test.log` and
`filesystem-path-final-test.log` on TB5). This diagnostics fix does not resolve
the stalled directory open.

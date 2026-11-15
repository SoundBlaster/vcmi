# TCP movement and request acknowledgement — 2026-11-10

## Resource removal and reload — 2026-11-15

The lobby-start test now identifies the gold object at `(35,35,0)` before
the red hero visits it across the northwest corner. The server and both
client replicas initially contain that object. After the visit, all three
have 1,000 more gold for red and no longer contain the object. The test also
checks the object's absence and red's current gold on the original server and
both replicas before saving, then on the reloaded server and two newly
connected replicas. Later turn income can change the balance before the save.

The expanded test passed three consecutive repetitions on macOS arm64 using
the TB5 `vcmitest` built from `503ac6d` plus the test change. The XML report
`runtime/tcp-resource-replication-2026-11-15.xml` contains the last repetition.
The server and listeners run in one process and exchange actual TCP loopback
packets; this does not compare the entire game state or establish behavior on
remote hosts or in a graphical client. The build succeeded with duplicate
library and missing Conan search-path warnings.

A `CVCMIServer` and two test listeners negotiated the experimental toroidal
capability over separate TCP loopback connections. The test installed a
two-level toroidal game state and assigned one player to each connection;
this is a controlled fixture, not the normal lobby-start or graphical-client
flow. Each listener loaded the same serialized starting state as a replica.

The second connection sent a movement request claiming the first player's
hero. The server rejected it and the hero stayed at the origin. The first
connection then sent the same valid corner-crossing request. The actual
server `TryMoveHero` packet reached both listeners, and each replica matched
the authoritative position and movement points; both replicas showed the
destination visible to the moving team and hidden from the other team. A
subsequent request by the owner to enter a rock tile failed and left the
hero in place. The second player then sent its own legal move across the
horizontal seam; both replicas matched its resulting position and movement
points and showed the destination visible to that player. These visibility assertions concern replicated game state,
not whether information in network packets is confidential.

The test exposed that `CGameHandler::handleReceivedPack` sent a successful
`PackageApplied` acknowledgement even after an action was rejected. It now
uses the visitor result. Both rejected requests produced a negative
acknowledgement; the valid request produced a positive one.

On macOS arm64, `vcmitest` built from `db84325` plus this change. The full
suite passed 1,419 tests, skipped two optional tests without their external
prototype fixtures, and retained four disabled tests. After adding the
blocked-terrain assertion and the second player’s move, the expanded TCP
test passed separately. The
suite uses test listeners and in-process server/game-state objects, although
their packets traverse real local TCP connections. It does not demonstrate
a full multiplayer lobby, graphical clients, separate machines, packet
confidentiality, or broader multiplayer behavior.

A separate bounded-map regression invokes the ordinary server request
handler with an owned hero. A rock tile returns `result=false` and leaves
the hero in place; an adjacent legal tile returns `result=true` and moves
the hero. This and the TCP scenario passed together on a build from
`f85437a` plus the bounded-map test. This provides a topology-independent
regression for the acknowledgement fix.

## TCP lobby start and turns

A second test connects two test listeners to a real `CVCMIServer` over TCP
loopback and negotiates the topology capability. It writes a small adapted
toroidal VMAP and configures its player slots directly in the server lobby;
the host then sends the ordinary `LobbyStartGame` packet. The server loads
the VMAP, starts the game and sends its initialized state to both listeners.
Both received a toroidal 36×36 map and the red hero at the expected corner.

With both listeners applying the server's client packets, the host sent the
corner visit: each state and the server gained 1,000 gold while the hero
remained at the starting tile. A second request crossed the corner. The host
then ended its turn; the guest became active and moved its own hero west
across the horizontal seam. Both replicas matched the server after each
step. The final test passed five consecutive runs on macOS arm64, built from
`8d71f38` plus the test. The real graphical clients, lobby UI, mixed-version
start, remote hosts and network confidentiality are outside this check.

## Recheck after pathfinding changes

After commit `4255569`, the TCP movement, TCP lobby-start and capability
negotiation tests each passed three consecutive times on macOS arm64. The log
is `tcp-current-4255569.log` on TB5. These remain in-process fixtures using
loopback TCP and test listeners; they are not graphical multiplayer sessions.

An older orphaned `vcmitest` process with the TCP movement filter had remained
running for hours and was excluded from the previous full-suite run. A macOS
sample showed its only thread waiting in `dyld` while opening a file, before
the test executable entered `main`. This is not evidence that the TCP test
itself hung; the path and cause of the `open` wait remain unknown. The current
TCP movement test passed independently as described above.

## Rejected seam move after normal lobby start

The TCP lobby-start test now places impassable rock at `(35,4,0)`. During
the blue player's turn, its listener requests a diagonal step there from
`(0,5,0)`, across the horizontal seam. The server returns a negative
`PackageApplied` result and both listeners receive a failed `TryMoveHero`.
The test checks the authoritative state and both replicas retain the hero's
position and movement points. It compares every surface tile's fog state
for both teams before and after the rejected move, confirms the three states
agree, then executes the legal westward step and compares fog again.

The expanded test passed three consecutive repetitions on macOS arm64 with the TB5 build from `ce3eb14`
plus the test. Both peers are test listeners in the same process as the server,
with packets over TCP loopback and directly configured lobby slots. The test
does not establish packet confidentiality, graphical multiplayer behavior,
remote-host behavior, other levels or all rejection reasons.

## Mixed-capability lobby start

The TCP lobby-start fixture now connects a third listener that offers the
current numeric version without the experimental topology tag. The server
negotiates the previous serialization version for that peer. When the host
requests the toroidal VMAP, it receives the incompatible-version message;
the server remains in the lobby without an initialized game and none of the
three listeners has received a game state. The third listener then sends its
normal disconnect packet. The same host successfully starts the toroidal
game with the two tagged peers and the original movement assertions run.

On macOS arm64, the test and nine enabled `GameConnectionTopologyTest` cases
passed three consecutive repetitions with the TB5 build from `68bfaf7` plus
this test change. The log is `tcp-mixed-capability-2026-11-10.log` in the
TB5 runtime directory. The untagged listener is a current-code test double, not an
old binary, and all three listeners and server run in one process over TCP
loopback. This does not resolve the disabled raw conflicting-layout test,
prove mixed-version graphical interoperability, or authenticate capabilities.

## Save and reload through a new TCP lobby

The same live TCP scenario now writes a server save after the established
movement and turn sequence. A second `CVCMIServer` and two new test listeners
connect over loopback TCP with the toroidal capability. The test reads the
saved header and start settings, assigns the new connections to the saved
players, then sends `LobbyStartGame` with `LOAD_GAME`. Both clients receive
the loaded toroidal state. The test compares both heroes' positions and
movement points and all surface fog cells of both teams with the saved
server state and with each other.

After loading, the blue player sends a normal movement request from
`(35,5,0)` to `(0,5,0)`. The server accepts it, spends movement points and
broadcasts the result. Both test clients and the server agree on the new
position, remaining points and both teams' surface fog. Temporary save and
configuration resources use an isolated filesystem overlay. The test
configures saved player slots directly and runs both server objects and all
test listeners in one process; TCP is loopback. It is not a graphical lobby,
remote-host session, network confidentiality test or evidence for all levels.

On macOS arm64, this expanded TCP test, the two-level disk-save/replica test,
and the enabled `GameConnectionTopologyTest` cases passed five consecutive
repetitions on the TB5 build from `41e3ddb` plus the test changes. The log is
`runtime/tcp-save-load-final-2026-11-14.log`. The raw conflicting-layout test
remains disabled and is not counted among those passes.

The full local suite subsequently passed 1,530 tests with no skips, including
the optional older save, VMAP and Complete-map fixtures; four tests remain
disabled. Its TB5 log is `runtime/full-suite-tcp-save-reload-2026-11-14.log`.

## Two levels after TCP save and reload

The TCP lobby fixture now creates both map levels and a second blue hero in
the underground. The server reveals different underground cells for red and
blue with ordinary fog packets. The fixture compares every fog cell for both
teams on both levels between the server and both replicas at selected surface
movement checkpoints, save and new-lobby reload. It also checks the underground
hero's position and movement points survive the disk save on the reloaded
server and both peers.

After reconnecting, blue moves its surface hero across an edge, then sends a
separate underground corner move from `(0,0,1)` to `(35,35,1)`. The server
accepts the request, and both peers agree on the underground hero's position,
remaining movement points and both teams' full fog. The test separately
asserts that red's fog on both levels and blue's surface fog do not change
when the underground hero moves. The selected initial underground reveals are
test setup, not movement-generated sight.

The extended TCP scenario, the server disk-save/replica scenario, and 15
enabled `GameConnectionTopologyTest` cases passed three consecutive runs on
macOS arm64: 17 tests per run. Results are
`runtime/tcp-underground-reload-2026-11-15.{log,xml}` under the TB5 VCMI
build root. The client listeners, lobby configuration and servers remain in
one process communicating over loopback TCP. This is not a remote-host or
graphical-client test, an audit of every state field, or evidence of packet
confidentiality. The test build retained existing linker warnings.

## Player-specific hero visibility after TCP fog packets

The same lobby scenario now checks the player-specific `getHeroes` callback
used by the minimap on both client replicas. The blue underground hero starts
at `(0,0,1)` and is hidden from red. The test asks the server to reveal that
tile for red and waits for ordinary fog packets on both peers; the hero then
appears in both replicas' red-player views. After the server hides the tile
and both replicas receive the update, the hero disappears from those views.
The blue-player view retains its own hero throughout. Existing checkpoints still
compare both teams' fog on both levels and exercise subsequent movement and
save/reload.

On macOS arm64, `vcmitest` built from `94e5130` plus this test, with existing
duplicate-library and missing Conan search-path linker warnings. The expanded
TCP test and `HeroMovementTest.MinimapHeroListRespectsFogAtToroidalCorners`
passed once and then three consecutive repetitions. Reports are
`runtime/tcp-hero-fog-2026-11-15.xml` and
`runtime/tcp-hero-fog-repeat-2026-11-15.{log,xml}` under the TB5 build root.
The server explicitly changes fog for the test rather than moving a sight
source, and the two test listeners share a process over loopback. This does
not test actual graphical minimap rendering, a remote host, or confidentiality
of network payloads; each replica contains a game state and the visibility
claims here concern the player-specific callbacks.

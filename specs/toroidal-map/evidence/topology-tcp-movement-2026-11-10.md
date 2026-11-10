# TCP movement and request acknowledgement — 2026-11-10

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

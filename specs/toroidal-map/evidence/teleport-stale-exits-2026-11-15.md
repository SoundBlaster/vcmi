# Revalidating teleport exits after a dialog

A teleport dialog records a snapshot of exits. In a server test, choosing a
two-way monolith exit occupied by a friendly hero caused an extra activity,
although `isExitPassable` rejected that exit. A separate protected-whirlpool
test captured a free exit, then added a boat there before replying: before the
fix, the hero and their boat moved onto the occupied exit. These cases were
reproduced by changing server state directly; this does not establish how
often ordinary multiplayer gameplay can produce either timing.

Monoliths now revalidate the selected object, position and passability against
current server state. Whirlpools revalidate the selected object/tile pair
against the current passable exits. If a selected destination is stale or no
exit is available, the server sends a failed movement packet so a client
following a route can stop instead of repeatedly requesting the old teleport.
The packet uses the canonical visitable cell, including when the hero's stored
anchor is an image across the seam. This changes bounded maps too.

The server tests cover a friendly hero occupying a selected two-way monolith
exit after the dialog was created, on both bounded and toroidal maps; the
toroidal test has both the entrance and occupied exit spanning the seam. It
checks the failed packet and that another free exit still works. Another test
places a boat on a selected wrapped whirlpool exit after the dialog and checks
both boat identity and position, plus the failed packet. Existing protected
and unprotected whirlpool success cases still pass. An extended gate test
confirms that friendly heroes can still open their exchange dialog through a
subterranean gate. The test server injects the position changes and captures
outgoing packets; it does not run a graphical or remote client.

On macOS arm64, all ten focused tests passed. The client, server and test
targets built successfully; the linker emitted duplicate-library and missing
Conan search-path warnings. The full local `vcmitest` run passed 1,930 tests,
skipped the external legacy-server test, and reported three disabled tests.
The XML and log are under `runtime/teleport-stale-exits-final-packets-2026-11-15.xml`
and `runtime/full-suite-teleport-final-2026-11-15.*` in the TB5 build root.
Actual client route stopping, remote multiplayer timing, other objects changing
while a dialog is open, and untested teleport configurations remain unverified.

## Reply ordering and another hero's movement

The focused tests now send `QuestionAnswer` through the actual server request
handler. A test server records the outgoing failure and request acknowledgement:
the failed `TryMoveHero` is emitted before the successful `PackageApplied`
reply for both the occupied monolith exit and whirlpool exit. This verifies
the order at the server callback boundary, not delivery over TCP or execution
by a graphical client. All ten focused tests passed on macOS arm64; the XML is
`runtime/teleport-reply-order-2026-11-15.xml` under the TB5 build root.

Source review found that `HeroMovementController::onTryMoveHero` stopped its
active movement on a stop packet for *any* visible hero. Such packets can be
broadcast for another player during simultaneous turns. The controller now
stops only when the packet's hero is the currently moving hero. The client
target built successfully with this change; no test or graphical session yet
exercises simultaneous clients receiving an unrelated hero's stop packet.

## Two TCP replicas

`HeroMovementTest.TcpPeersReceiveAuthoritativeCornerMovement` now adds three
ordinary two-way monoliths to its two-level toroidal fixture. The real server
and two listeners negotiate topology over separate framed TCP loopback
connections. After the earlier movement checks, a server setup packet places
the active hero near a monolith; a normal network movement request enters it
and sends a teleport question to the first peer. A second setup packet places
the same player's other hero on the selected exit at the edge after the
question was sent. Both packets are applied to the two replicas over TCP.

The first listener answers with that now-stale exit. Both listeners receive
the failed movement; on the first connection it precedes the successful
`PackageApplied` acknowledgement. The second listener does not receive that
acknowledgement. The server and replicas agree on the stationary hero,
movement points and occupied exit, and all fog cells for both teams on both
levels remain equal and unchanged after the answer. The TCP scenario and ten
related in-process teleport cases passed five consecutive repetitions on
macOS arm64. The full local suite passed 1,930 tests, skipped the external
legacy-server test, and reported three disabled tests. The repeat XML and
full-suite log/XML are `runtime/tcp-stale-teleport-repeat-2026-11-15.xml` and
`runtime/full-suite-tcp-teleport-2026-11-15.*` on the TB5 build root.

The fixture installs its game state and player slots directly; the listeners
and server run in one process. Exit occupancy is injected, rather than caused
by another player's actual move. These checks do not demonstrate graphical
client route stopping, a second machine, real simultaneous-player timing or
network confidentiality. The observed ordered delivery is on the requesting
TCP connection; no order is claimed between independent connections.

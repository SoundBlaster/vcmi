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

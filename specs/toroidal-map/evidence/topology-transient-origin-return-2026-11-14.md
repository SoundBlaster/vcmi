# Conditional emergency return from transit

A server records the confirmed LAND tile when an on-foot hero enters AIR or
WATER transit on a toroidal map. If no ordinary safe known landing path is
available, an expired timer or manual end-turn request can return a single
transient hero to that exact tile, consume remaining movement points and notify
the owning player. This does not choose an arbitrary known location.

The return requires the tile to remain passable, unguarded, known and free of
blocking or other visitable objects; a nonblocking event is permitted without
being visited. The destination sight area and guard neighborhood must already
be known. Recovery uses a replicated teleport packet without departure or
arrival object callbacks, so an event under the hero or at the origin is not
activated. The movement activity brackets the packet as in normal movement.

The origin is versioned in server save state. The immediately preceding
version loads without one; it does not invent a location. A removed or
relocated hero loses its stored origin, as does a hero that lands normally.
At this revision the implementation did not cover multiple simultaneous transient
heroes; a later [group recovery change](topology-multiple-transient-return-2026-11-14.md)
handles a constrained subset. Old saves without an origin, an occupied or
unsuitable origin, vehicles and other hidden-object and dynamic-world cases remain. It therefore cannot guarantee that a
timer or AI turn always finishes. G12 remains pending.

`HeroMovementTest.StrandedWaterWalkerReturnsToTheLandWhereTransitStarted`
replays the fog-exploration case, checks rejected recovery when the origin is
hidden, water or occupied, checks that a previous-version server archive does
not fabricate an origin, and after a current-version memory serialization
verifies the timer-driven return with zero movement points. Probe objects check
that source leave and destination event callbacks do not run. The fixture does
not itself prove an on-disk save/load, real timer scheduling or GUI behavior.

The TCP loopback lobby test moves a hero into AIR on the server, artificially
reduces its movement points to one, then sends a manual end-turn request. Both
peers observe the return to the recorded origin, LAND layer, zero points and
turn transfer. The artificial point reduction tests the networking path, not
natural stranding or remote hosts.

On macOS arm64, `vcmitest` and `vcmiserver` built. The full suite with the
prototype save, VMAP and legacy H3M fixtures passed 1,497 tests from 226 suites
without skips; four remain disabled. The full-run log is
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/full-suite-transient-origin-return-tcp-2026-11-14.log`.
The fixture was subsequently strengthened to require a nonempty army and a
spellbook and check both after recovery; the strengthened case passed separately.

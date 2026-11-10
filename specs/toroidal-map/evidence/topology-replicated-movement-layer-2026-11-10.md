# Replicated movement layer and replanning

A hero flying over a restricted object could legally continue across a seam
on the server, but the ordinary pathfinder initialized its search on LAND and
could not find that continuation. The extended source-exit fixture reproduced
this failure before the change. Successful movement now carries its confirmed
destination layer in the versioned `TryMoveHero` packet and stores it on the
hero in game state. Ordinary pathfinding and Nullkiller2's hero actor use that
layer on toroidal maps; bounded maps retain their previous initial-layer rule.
Failed movement does not change the layer. A `ChangeObjPos` packet resets it
to unknown, and the network-lag rollback visitor restores the previous value.

The source-exit test replans while airborne, exercises a predicted landing and
its actual rollback, and verifies landing changes the initial layer to LAND.
A TCP-loopback fixture sends an AIR step from the authoritative server to two
replicas; both replan from AIR and then observe LAND after a later crossing.
The AI actor test checks AIR, WATER, bounded, and unknown starting layers; it
does not run a complete AI turn. These are controlled in-process fixtures,
not graphical clients or remote hosts.

`TOROIDAL_REPLICATED_MOVEMENT_LAYER` adds the hero and packet fields with a
new branch-local serialization version. The save test writes and reads a
real current-format airborne save. It also constructs a previous-version
save whose layer existed only in server state and confirms that normal server
load migrates that state into the hero. Older saves without either field use
unknown and retain the previous fallback. Ordinary bounded saves continue to
use the prior bounded version. These formats and the experimental lobby tag
are not upstream allocations.

To prevent an older tagged peer from starting a toroidal session without the
field, the server start guard requires the replicated-layer version. A new
client rejects an older tagged experimental server at greeting time, including
for bounded maps; it continues to accept sufficiently old untagged servers
for bounded sessions. Packet round trips and server guard tests cover these
rules; actual older binaries and graphical mixed-version sessions are not
tested. A tagged peer's claim is not authentication.

On macOS arm64, client, server, editor, AI and `vcmitest` targets built from
`099e5cf` plus this change. The full local suite passed 1,470 tests from 226
suites without skips; four remain disabled. It used the prior untagged
prototype save and VMAP and the Complete `Dead and Buried.h3m`. The log is
`full-suite-replicated-movement-layer-final-2026-11-10.log` in the TB5 runtime
directory. Existing duplicate-library and missing Conan search-path linker
warnings remain. The disabled raw codec layout mismatch test is not resolved.

This does not establish actual GUI behavior, remote multiplayer, every
relocation, interruption or turn boundary, Flight expiration, or Water Walk
replanning on a real map. Those remain open verification and design cases.

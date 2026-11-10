# Ending a turn during transient movement

The ordinary movement controller does not stop a route on AIR or WATER nodes,
but the server's `EndTurn` handler previously accepted an explicit request
while a hero was in AIR. The expanded real TCP-loopback lobby test reproduced
that acceptance before this change. For toroidal games, the turn processor now
rejects ending the turn while any hero owned by that player has a confirmed
AIR or WATER layer. The automatic turn timer checks the same predicate before
attempting to end a turn. Bounded games retain the previous behavior.

`ToroidalDemoVmapTest.TcpLobbyStartsToroidalGameForTwoPeers` starts a normal
lobby session with two test listeners, gives the red hero a one-day Flight
bonus, flies across an edge and receives a negative acknowledgement for an
early end-turn request. The server and both replicas remain on the red turn.
After landing, the end-turn request succeeds; blue completes its turn and
Flight expires on the next day on all three states. The test passed three
consecutive repetitions.

`HeroMovementTest.TransientMovementGuardChecksAllOwnedHeroes` exercises a
real Water Walk step, checks player isolation, checks that another owned hero
is considered, and compares bounded behavior. The second hero's AIR layer is
set directly in this fixture. The automatic timer's actual countdown was not
exercised. Unknown layers in older saves, aviation vehicles, graphical play,
remote peers and bonus removal outside this tested day transition remain open.

The ordinary client plans routes to a LAND or SAIL stopping point. A malformed
or interrupted request could nevertheless exhaust movement while the hero is
in AIR or WATER; the new guard can then prevent ending the turn without a
recovery mechanism. A subsequent server check rejects a standard toroidal AIR
or WATER step that would consume all remaining movement points. The
`HeroMovementTest.ToroidalTransientStepCannotConsumeLastMovementPoints` test
reproduced the exhausted AIR state before the check and now covers both AIR
and WATER with 100 and 150 points, and a successful WATER crossing and return
with more points. `BoundedTransientStepKeepsExistingLastMovementRule` checks
the prior behavior on a bounded map.

The last-points check alone did not guarantee reachability: a two-step WATER
example still stranded the hero with 100 points. A subsequent partial
[known-landing check](topology-known-landing-2026-11-10.md) rejects that
particular second step when the relevant area is known. Recovery for other
cases, including older saves and timer expiry, remains required for G12.
The client, server and tests built successfully with the safeguard on macOS
arm64. The full suite passed 1,473 tests from 226 suites without skips; four
remain disabled. The prototype save and VMAP and the Complete legacy map
were supplied. The log is `full-suite-transient-last-step-2026-11-10.log` in
the TB5 runtime directory. This does not establish graphical play or remote
network behavior for the new rejection.

On macOS arm64, `vcmitest`, `vcmiserver` and `vcmiclient` built from `7d1fa3f` plus this
change. The full suite passed 1,471 tests from 226 suites without skips; four
remain disabled. The optional prior prototype save and VMAP and the Complete
`Dead and Buried.h3m` were supplied. The log is
`full-suite-transient-turn-guard-2026-11-10.log` in the TB5 runtime directory.

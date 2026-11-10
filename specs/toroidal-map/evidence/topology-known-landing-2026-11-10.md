# Known landing check for transient movement

The earlier last-points rejection did not cover a hero who kept points but
could no longer reach land. On an on-foot toroidal AIR or WATER step, the
server now compares a known landing from the current state with a search from
the proposed state. If the current state has a known safe stopping route and
the search can establish that the proposed state has none, the move is
rejected without spending points. The search uses the player's fog of war;
when an unknown reachable frontier could change the answer, it does not reject
on that basis. If the hero already lacks a known route, this check does not
prevent exploration. The earlier rejection of a step that spends all points
on AIR or WATER remains in effect.

The check is intentionally incomplete. It does not apply to boats or every
legacy unknown-layer situation. It initially skipped heroes with both Flight
and Water Walk; the later [layer-switch change](topology-transient-switches-2026-11-10.md)
models the tested on-foot transitions and removes that exclusion. The
prospective search cannot predict all object-leave effects,
newly revealed cells or concurrent world changes. A previously stranded hero
can still be unable to end the turn, including when a turn timer expires.
G12 remains pending; this is not a general recovery guarantee.

`ToroidalWaterWalkRejectsStepThatLosesKnownLanding` and
`ToroidalFlightRejectsStepThatLosesKnownLanding` cover the reproduced unsafe
second step and a successful return. `ToroidalWaterWalkMayUseLastPointsToReachKnownShore`
covers a legal route forward. `UnknownShoreDoesNotChangeTransientMoveAcceptance`
compares acceptance when a hidden tile changes terrain, and
`AlreadyStrandedWaterWalkerCanContinueExploring` covers an already unsafe
state after the map changes. These are local synthetic server tests.

`ToroidalDemoVmapTest.TcpLobbyStartsToroidalGameForTwoPeers` also exercises the
known-landing rejection over a real TCP loopback lobby and two connected test
peers. The hero flies across the seam, then requests a diagonal AIR step that
costs less than its remaining movement but loses the known landing route. The
server sends a failed movement and negative acknowledgement; its hero and both
replicas retain the same position, movement points and red-player fog. In this non-Goldmaster
build, the host also receives the specific known-landing diagnostic. The
extended test passed three consecutive runs. This does not prove behavior
over a remote network or in the graphical client.

The ordinary pathfinder had also discarded a legal last-point transition
from AIR or WATER to LAND, despite the server accepting it. The common
movement-cost rule now accepts an actual same-turn landing. Flight, Water Walk
and a bounded-map Water Walk fixture exercise this; the bounded fixture uses
an explicit prospective start because its transit layer is not replicated.

On macOS arm64, `vcmitest`, `vcmiserver` and `vcmiclient` built, and the full
suite passed 1,484 tests from 226 suites without skips; four remain disabled.
The optional prototype save, VMAP and Complete legacy map were supplied.
`ToroidalTransientLandingSearchWorksOn252Map` completed five crossings and
returns on a revealed 252 × 252 surface map in 36–37 ms in two targeted test
runs, including fixture setup. This is not a multiplayer load benchmark or a
worst-case performance bound. The full log is
`full-suite-known-landing-tcp-2026-11-10.log` in the TB5 runtime directory;
that run includes the extended TCP test before the additional fog assertions;
the final TCP test with those assertions passed three targeted runs.

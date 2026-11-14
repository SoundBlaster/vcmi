# Timer recovery after live transient movement

A toroidal on-foot hero entering AIR or WATER transit could disable the legacy
end-turn gate. The expired timer consulted that gate before attempting the
existing safe-landing routine, so the routine never ran. The earlier test
initialized the timer only after placing the hero in transit and therefore
reset the gate and missed this sequence.

For toroidal on-foot AIR or WATER transit the movement handler now leaves
the timer gate open. The timer still checks activities and requires all
transient heroes to land before transferring a turn. Bounded-map movement
keeps the existing gate behavior. This is not a general stranded-hero solution:
if no known safe route or in-place landing exists, the timer can still wait
indefinitely.

`HeroMovementTest.ExpiredTimerLandsHeroWhoEnteredWaterDuringTheTurn`
initializes a one-millisecond timer first, walks from land across a seam into
water, and drives the timer twice. Before the fix the hero stayed on water;
afterward the hero lands with the ordinary movement cost. Existing tests also
check that an explicit gate prevents auto-landing and that missing routes wait.

`ToroidalDemoVmapTest.TcpLobbyStartsToroidalGameForTwoPeers` now initializes
the timer before a network Flight move. Direct timer updates cause landing and
turn transfer, and both loopback clients see the same hero state and new
acting player. The second client acknowledges the normal turn-start pause.
This test directly drives updates; it does not verify wall-clock scheduling,
remote hosts, or animation.

On macOS arm64, the focused eight tests passed. The full `vcmitest` run with
the prototype save, VMAP and legacy H3M fixtures passed 1,496 tests from 226
suites with no skips; four tests were disabled. Log:
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/full-suite-timer-real-order-2026-11-14.log`.
The G12 acceptance scenario remains pending for stranded heroes and other
untested cases.

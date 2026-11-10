# Timer expiry during transient movement

When a toroidal player's timer has expired and ending the turn is allowed,
the server now looks for an already-known route free of visitable objects and guards
from an on-foot AIR or WATER hero to LAND. It selects a lowest-cost candidate
found by the existing pathfinder, advances one normal server movement step
per timer update, and checks activities and the world again on the next
update. Movement points are charged normally. Once no owned hero remains in
transit, the normal timer path can end the turn. This does not add movement
points or teleport a hero. If an attempt finds no suitable path, another
attempt is delayed by one second of timer updates; this transient throttle is
not stored in saves and a restored session can retry immediately.

The pathfinder's regular route knowledge and the additional known-neighborhood
check are used; automatic movement avoids visitable objects, events and
guarded tiles along its chosen path. This policy is intentionally limited:
boats, unknown paths, a route requiring an object interaction, and a hero
with no available safe path may still leave the timer waiting indefinitely.
Neither automatic relocation nor recovery for those cases is implemented.

`AutomaticLandingAdvancesOneOrdinaryStepAtATime` checks a two-step WATER
return with normal costs. `AutomaticLandingDoesNotTriggerAnEvent` verifies
that an otherwise reachable event tile is not used automatically.
`AutomaticLandingCanSkipAStrandedHero` checks that a second hero can land
while the first has no path. `ExpiredTimerWaitsUntilAutomaticLandingIsAllowed`
checks the existing end-turn gate, and `ExpiredTimerWaitsWhenNoSafeLandingExists`
checks that the hero and turn remain unchanged until a route appears, then
retries after the delay.

The extended `ToroidalDemoVmapTest.TcpLobbyStartsToroidalGameForTwoPeers`
sets up an active one-millisecond timer on a running server, explicitly calls
the timer update to exhaust it while a hero is airborne over water, and checks
that an ordinary neighboring landing is replicated to two TCP loopback peers
with the correct movement-point cost. A subsequent timer update advances the
turn. The test sets the timer after session start and drives updates directly;
it does not establish real wall-clock scheduling, remote-network behavior or
graphical animation. The targeted timer and TCP tests passed three repetitions
before the final additional multi-hero test, which passed separately.

This work also fixes a general timer-loop error: missing player slots are
skipped instead of requesting their hero lists. The two-player TCP countdown
previously threw while looking up an inactive slot, and now completes.

On macOS arm64, `vcmitest`, `vcmiserver` and `vcmiclient` built successfully.
The full suite passed 1,492 tests from 226 suites without skips; four remain
disabled. The prototype save, VMAP and Complete legacy map were supplied.
The log is `full-suite-timer-auto-landing-2026-11-10.log` in the TB5 runtime
directory. G12 remains pending until a recoverable outcome is established
for the remaining stranded states.

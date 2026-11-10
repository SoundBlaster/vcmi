# In-place landing when a player ends a turn

On a toroidal map, a player could be unable to end a turn while a hero was in
`AIR`, even when the hero was already above clear land. The earlier timer
recovery could land such a hero in place; the manual end-turn path still
rejected the request.

The server now checks every owned hero in a transient layer before ending the
turn. It lands them in place only when every such hero satisfies the same
known, unguarded, clear-land conditions as the timer recovery. If any hero
cannot be landed in place, the request is rejected and no hero is landed by
this operation. This does not automatically move a hero along a route and does
not solve general stranded-hero recovery. Normal turn and activity checks run
before the landing decision.

`InPlaceLandingChecksAllHeroesBeforeChangingAny` sets two airborne heroes on
land and hides a neighboring cell of one. The decision rejects both without
changing their layers; after revealing the cell, both land at their original
positions. The TCP loopback lobby test establishes a synthetic airborne state
above clear land, submits an actual `EndTurn` request, and observes both
connected replicas receive the same-position `LAND` update with unchanged
movement points and the turn pass to the opponent. The focused tests passed.
This establishes the tested local server and TCP packet flow, not remote
network conditions or GUI behavior.

On macOS arm64, the server and test executable built on TB5. The full suite
passed 1,494 tests from 226 suites without skips; four tests remain disabled.
The prototype save, VMAP and Complete legacy map were supplied. The log is
`runtime/full-suite-manual-in-place-landing-2026-11-10.log` on TB5.
`specpm validate . --json` reported zero errors and warnings.

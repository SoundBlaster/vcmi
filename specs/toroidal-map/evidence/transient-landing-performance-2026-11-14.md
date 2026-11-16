# Transient landing performance

`HeroMovementTest.TransientRouteSafetyWorksOnLargeMap` has an opt-in timing
probe enabled with `VCMI_TOROID_LANDING_BENCH=1`. It uses a revealed 252×252
toroidal map with one land tile and Water Walk. Map construction is excluded
from the measurements. The single-search case starts on adjacent water and
uses the real `findKnownTransientLanding` with `PLAYER_ROUTE`; the route case
times `getPath` after building a fresh path graph. It then requests a target
one step farther along the same verified predecessor chain. Each case drops
one warmup; single-search medians use 11 samples and route medians use five.

On macOS arm64, Apple M4 Pro, Release build at `1283636` without shared-prefix
caching, the observed medians in milliseconds were:

| Movement points | One landing search | First route, tiles | First route | Next shared-prefix route |
| ---: | ---: | ---: | ---: | ---: |
| 1,200 | 0.73 | 5 | 3.00 | 3.44 |
| 3,000 | 2.75 | 10 | 19.55 | 20.79 |
| 6,000 | 10.13 | 20 | 140.70 | 144.81 |

The 6,000-point case is a stress case, not a claim about typical hero stats.
These are local wall times in one synthetic configuration; they do not measure
frame pacing, server concurrency, other maps, or remote multiplayer. The
results support investigating repeated shared-prefix work, not a claim that
every game experiences a visible pause.

## Shared-prefix cache

The path graph now caches the landing status after each successfully checked
prefix node, under the existing route-safety mutex. A new search, reuse or
reconfiguration clears the cache. The current-turn and interactive-action
boundaries still stop validation; an unsafe extension is checked and rejected
even when its preceding prefix was accepted.

The same local probe after the change produced these medians, in milliseconds:

| Movement points | One landing search | First route, tiles | First route | Next shared-prefix route |
| ---: | ---: | ---: | ---: | ---: |
| 1,200 | 0.77 | 5 | 3.15 | 0.51 |
| 3,000 | 2.86 | 10 | 20.36 | 1.34 |
| 6,000 | 10.29 | 20 | 145.22 | 4.56 |

The first route and individual landing search remain expensive in this stress
fixture; this change reduces repeated validation only when paths share nodes
in the same graph. It does not cache server searches across movement packets.
The route-agreement test validates a safe prefix before eight concurrent
readers request the unsafe extension, then recalculates with changed movement
points. Twenty-nine related landing, fog and route tests passed three runs.
An additional check compared 21 destination results from a populated cache
with independently recalculated graphs, including accepted and rejected
routes; the expanded route-agreement test passed three runs.
These observations do not establish frame pacing or a general performance
bound.

With the three external fixtures configured, the complete local `vcmitest`
run passed all 1,541 tests before the additional query-order assertions;
four tests remained disabled. The TB5 log is
`runtime/full-suite-route-prefix-cache-2026-11-14.log`. SpecPM validation
reported no errors or warnings. The macOS arm64 client and server targets
also built; the linker reported existing duplicate-library and missing-search-
directory warnings. This does not verify a running client or server.

## Reusing a landing-search workspace

Within a single route validation, landing searches now reuse an exclusively
owned graph allocation. Each search resets its workspace and reconstructs the
path graph with the current start, movement points and knowledge policy. The
workspace is not shared between route validations or server requests.

The same local timing probe after this change produced these medians, in
milliseconds:

| Movement points | One landing search | First route, tiles | First route | Next shared-prefix route |
| ---: | ---: | ---: | ---: | ---: |
| 1,200 | 0.70 | 5 | 1.58 | 0.48 |
| 3,000 | 2.80 | 10 | 16.65 | 1.29 |
| 6,000 | 10.02 | 20 | 135.89 | 4.64 |

These are separate local runs, not a statistical performance guarantee. The
first route is still costly in the 6,000-point stress case. The single-search
measurement does not reuse a workspace. No game frame pacing or server
concurrency was measured.

The route-agreement test compares fresh and reused workspaces at multiple
starts, with both knowledge policies and a changing fog tile; recovery routes
are compared as well. Twenty-nine related tests passed three runs. With all
three external fixtures configured, the complete suite passed 1,541 tests;
four remain disabled. The log is
`runtime/full-suite-landing-scratch-2026-11-14.log` on TB5. Client and server
targets built on macOS arm64, with the existing linker warnings noted above.

## Stop after a settled known landing

When only the existence of a known landing is requested, the pathfinder can
now stop after it removes a qualifying land node from its priority queue. Its
predecessor chain is already settled and must satisfy the same visibility,
movement-point and interaction checks as the complete search. If no such
queued node is found, the search completes and the existing scan handles
terminal nodes and the unknown-frontier result. Recovery requests for an
actual plain route still build and scan the complete graph. Queue links are
cleared on early return before a scratch graph can be reused.

On 2026-11-15, the same opt-in probe was run before and after this change on
the same Mac, in separate local runs. Median times, in milliseconds:

| Movement points | One landing before | One landing after | First route before | First route after |
| ---: | ---: | ---: | ---: | ---: |
| 1,200 | 0.74 | 0.32 | 1.56 | 0.48 |
| 3,000 | 2.62 | 0.33 | 16.08 | 1.45 |
| 6,000 | 9.55 | 0.31 | 129.49 | 7.86 |

The logs are `runtime/landing-early-baseline-2026-11-15.log` and
`runtime/landing-early-after-2026-11-15.log` on TB5. The land tile in this
fixture is readily reachable, so these results do not bound searches where a
landing is distant or unknown. They do not measure frame pacing, actual
server concurrency, or multiplayer latency.

The route-agreement test also compares early results with full plain-route
searches on an object-free water map, including limited visibility, both
knowledge policies, and starts near opposite edges. It verifies that an
interrupted search clears heap links and that its graph can then be reused
for a complete search.

The macOS arm64 client, server and test targets built. With the prototype
VMAP and save and the Complete H3M fixtures supplied, the full local suite
passed 1,661 tests; four tests were disabled. The test log is
`runtime/full-suite-landing-early-2026-11-15.log` on TB5. SpecPM validation
reported no errors or warnings. The build retained existing linker warnings
about duplicate libraries and absent search directories.

## Distant and unknown landing

The opt-in probe also measures a single search from water at different
distances from the only land tile. The map is still a revealed 252×252 torus,
and each timed call allocates its own graph; map construction is excluded.
These are `PLAYER_ROUTE` searches, not measurements of a server request. On
the same Mac in a Release build based on `1159bf1`, three iterations in one
process gave the following ranges of per-iteration medians. Each median uses
seven timed samples after one warmup:

| Movement points | Distance to land | Result | Median range, ms |
| ---: | ---: | --- | ---: |
| 1,200 | 1 | found | 0.31–0.33 |
| 1,200 | 10 | found | 0.57–0.59 |
| 1,200 | 20 | none | 0.84–0.86 |
| 3,000 | 30 | found | 2.59–2.64 |
| 6,000 | 60 | found | 9.69–9.80 |
| 6,000 | 100 | none | 9.98–10.09 |
| 6,000 | 125 | none | 9.96–10.18 |

For a separate start surrounded by a 9×9 revealed water area, with the land
tile hidden beyond the fog, the result was `UNKNOWN`: median searches ranged
from 0.48 to 0.53 ms across the three movement budgets and iterations. The
ordinary test also asserts found, none and unknown results independently of
the opt-in benchmark. The log and XML are
`runtime/landing-distant-final-2026-11-15.log` and
`runtime/landing-distant-final-2026-11-15.xml` on TB5.

The 6,000-point budget is a stress case. These measurements do not establish
typical game latency, a worst-case bound, frame pacing, or the combined cost
of all checks in a server movement request.

The probe also requests longer routes from fresh, already calculated path
graphs. It times only `getPath`, including route-safety checks but excluding
the initial graph calculation. Three iterations in one process produced these
ranges of medians, each from five samples after a warmup:

| Movement points | Destination distance | Route result | Median range, ms |
| ---: | ---: | --- | ---: |
| 3,000 | 15 | accepted | 3.64–3.76 |
| 3,000 | 20 | rejected | 4.31–4.43 |
| 6,000 | 30 | accepted | 25.59–26.08 |
| 6,000 | 40 | rejected | 27.86–28.06 |

Before route-safety validation, all four destinations were reachable in the
current turn. A destination 60 tiles away at 6,000 points was already
unreachable in the raw graph and its immediate rejection does not measure a
landing search. These are synthetic first requests, not repeated cached
requests or end-to-end UI timings. The log and XML are
`runtime/landing-distant-routes-final-2026-11-15.log` and
`runtime/landing-distant-routes-final-2026-11-15.xml` on TB5.

## Selected water tile on Earth

On 2026-11-16, the opt-in `VCMI_INSPECT_EARTH_LANDING_PERFORMANCE` check
initialized the current 252×576 Earth demo, SHA-256
`f8add693e77bed3ee423481f9ecbb81ed9e8ddb75ecbc0d6161ca006c60e33b5`, with
its HotA profile. It revealed the surface, gave the red hero Water Walk in
memory and searched from the water tile `(239,425,0)` using `PLAYER_ROUTE`.
Map initialization and fog preparation were outside the timing; each call
allocated its normal search graph. For movement budgets 1,200, 3,000 and
6,000, all searches returned `FOUND`. Three independent processes, each
discarding one warmup and timing seven searches per budget, yielded median
ranges of 0.731–0.751 ms, 0.715–0.722 ms and 0.714–0.724 ms respectively.

All three checks passed on macOS arm64 with a Release test build based on
`6bdaab7` plus the diagnostic. XML:
`runtime/earth-landing-performance-final-{1,2,3}-2026-11-16.xml` on TB5.
This measures one selected candidate and one search, not a full server move,
frame time, multiplayer contention, fog-limited play or a worst-case bound.
It does not justify removing the performance limitation for other routes.

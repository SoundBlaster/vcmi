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

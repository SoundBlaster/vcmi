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

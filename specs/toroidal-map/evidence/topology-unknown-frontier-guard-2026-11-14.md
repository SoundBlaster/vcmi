# Preventing transit stranding at an unknown frontier

On a toroidal map, when an on-foot hero with Flight or Water Walk can
currently reach a known landing, the server now rejects a transit step unless
it can also find a known safe landing from the proposed position with the
remaining movement points. A reachable unknown frontier is not by itself a
landing. A failed step does not spend movement points. This is a change from
the earlier guard, which rejected NONE but accepted UNKNOWN.

The search considers the player's explored tiles and conservatively requires
route and guard neighborhoods to be known. Thus it can reject a step even if
an actual landing exists beyond fog or a partially unknown guard neighborhood.
The intended result must not depend on hidden terrain; the paired test below
checks one such case but is not a general confidentiality proof. An already
stranded hero is still permitted to explore: this guard prevents loss of a
known landing, and does not by itself repair old saves, interruptions, or
world changes. Existing recovery remains necessary.

`FlightStopsBeforeStrandingNearFog` and `WaterWalkStopsBeforeStrandingNearFog`
cast the actual spells on a single-island, partially revealed map and check
that direct transit steps are rejected before the hero loses its known return
route. They then use ordinary automatic landing steps to return to the island.
`UnknownShoreDoesNotChangeTransientMoveAcceptance` compares a hidden shore
with hidden water and expects the same rejection. `VisibleShoreRemainsReachableNearFog`
constructs one visible route through an otherwise partially hidden corridor;
the pathfinder finds it and server movement reaches the shore. These are
local game-handler tests, not interactive exploration.

The TCP lobby movement test passed, but does not exercise this exact unknown
frontier rejection on two replicas. The current client and server do not yet
share a demonstrated guarantee that every displayed route will survive the
conservative server check. GUI feedback, cursor scouting, other partially
revealed layouts, path invalidation during movement, old saves and general
recovery remain unverified.

On the macOS arm64 TB5 build, client, server and test executable built. The
movement suite and TCP lobby test passed 105 cases. The full suite ran 1,505
tests: 1,502 passed and three optional external-fixture tests were skipped;
four tests were disabled. The complete log is
`full-suite-strict-known-landing-2026-11-14.log` in the TB5 runtime directory.

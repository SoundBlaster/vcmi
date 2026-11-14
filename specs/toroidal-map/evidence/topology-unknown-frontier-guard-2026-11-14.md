# Preventing transit stranding at an unknown frontier

On a toroidal map, when an on-foot hero with Flight or Water Walk can
currently reach a known landing, the server now rejects a transit step unless
it can also find a known safe landing from the proposed position with the
remaining movement points. A reachable unknown frontier is not by itself a
landing. A failed step does not spend movement points. This is a change from
the earlier guard, which rejected NONE but accepted UNKNOWN.

For normal movement, the search uses explored route tiles and guards known to
the player. Automatic recovery retains the stricter requirement that guard
neighborhoods along its route are known. The movement guard can reject a step
even if an actual landing exists beyond fog. The intended decision must not
depend on hidden terrain or guards; the paired tests below check specific
cases but are not a general confidentiality proof. An already
stranded hero is still permitted to explore: this guard prevents loss of a
known landing, and does not by itself repair old saves, interruptions, or
world changes. Existing recovery remains necessary.

`FlightStopsBeforeStrandingNearFog` and `WaterWalkStopsBeforeStrandingNearFog`
cast the actual spells on a single-island, partially revealed map and check
that direct transit steps are rejected before the hero loses its known return
route. They then use ordinary automatic landing steps to return to the island.
`UnknownShoreDoesNotChangeTransientMoveAcceptance` compares a hidden shore
with hidden water and expects the same rejection. `VisibleShoreRemainsReachableNearFog`
constructs a 20-step visible route through an otherwise partially hidden
corridor; the pathfinder finds it and server movement reaches the shore.
`HiddenGuardDoesNotChangeTransitSafetyDecision` compares acceptance with and
without a hidden guard alongside such a route. These are local game-handler
tests, not interactive exploration.

The initial implementation required known guard neighborhoods even for
normal movement. It rejected the 20-step visible route at an intermediate
tile; the current distinction preserves that route. A guard revealed later
can still invalidate a route, so this does not eliminate the need for
interruption handling and recovery.

The TCP lobby test now also checks unknown-frontier rejection on two replicas.
The test directly establishes an identical hidden adjacent tile on the server
and replicas: it is a synthetic state, since normal fog packets reveal cells
observed by a hero. The movement request and failed acknowledgement travel
over TCP. Both replicas receive the failed move and retain the same position,
movement points and fog as the server. The test restores visibility through a
normal network packet and continues the game. It passed three consecutive
runs. As a negative control, temporarily restoring the former NONE-only check
made this particular request succeed and the test fail; the current guard was
then restored and rebuilt.

The current client and server do not yet share a demonstrated guarantee that
every displayed route will survive the conservative server check. GUI
feedback, cursor scouting, naturally reached unknown-frontier rejection,
other partially revealed layouts, path invalidation during movement, old saves
and general recovery remain unverified.

On the macOS arm64 TB5 build, client, server and test executable built. The
full suite after the route-knowledge change ran 1,506 tests: 1,503 passed and
three optional external-fixture tests were skipped; four tests were disabled.
The complete log is `full-suite-known-route-scouting-2026-11-14.log` in the
TB5 runtime directory.

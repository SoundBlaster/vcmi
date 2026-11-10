# Prospective transient pathfinding

`SingleHeroPathfinderConfig` can start a search from a proposed on-foot AIR
or WATER position and movement balance on the hero's current level. This does
not move the hero or change map occupancy. A read-only tile view excludes the
hero's current footprint while preserving other objects, so a path can return
to the tile the hero is expected to leave. The player's current fog of war is
used; the search does not grant visibility of the proposed move's future sight.
The overload rejects a boat, a different level, an invalid position, another
start layer or negative movement points.

The test `ProposedWaterWalkStartMatchesActualStateAcrossSeam` compares all LAND
and WATER nodes for current-turn reachability and remaining movement, first
from a proposed position and then after the server actually performs the step.
Without the tile view, it exposed a false negative when the hero occupied the
old tile. `ProposedFlightStartKeepsUnderlyingObjectAndMatchesActualState`
performs the analogous comparison for LAND and AIR with an underlying object
and a hidden cell; it also verifies that the overlay leaves the original tile
unchanged. These are small synthetic maps, not exhaustive equivalence tests.

The view is not a complete simulation of a move: leaving objects may produce
effects, newly revealed cells are intentionally absent, and other actors or
the map may change. Boats, inter-level changes, arbitrary initial layers and
all object interactions are outside the validated scope. No server acceptance
decision uses this search yet; it is preparation for investigating stranded
heroes, not a recovery guarantee or a resolution of G12.

On macOS arm64, `vcmitest`, `vcmiserver` and `vcmiclient` built successfully.
The full suite passed 1,477 tests from 226 suites without skips; four remain
disabled. The optional prototype save, VMAP and Complete legacy map were
supplied. The log is `full-suite-prospective-pathfinder-2026-11-10.log` in
the TB5 runtime directory. GUI play, remote multiplayer and the performance
cost of a server-side search per step are not established by these tests.

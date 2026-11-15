# Nullkiller2 nearest-town planning

The unskilled Town Portal planner independently selects a nearest town for
each source node. It now uses the map topology's shortest horizontal
displacement, preserving the spell's existing cross-level 2D comparison and
bounded-map behavior. This aligns distance selection with the game spell;
other differences in the candidate-town pool are not covered here.

`Nullkiller2_AIGateway.plansUnskilledTownPortalToNearestTownAcrossSeam`
constructs a hero at `(0,10,0)` and two friendly towns on a 36-wide map.
With only enough movement to cast in the current turn, it checks a same-turn
AI path containing Town Portal to the interior town on a bounded map, and to
the town across the seam on a toroidal map. The toroidal case failed before
the change and passed afterward. The full focused run passed 36 tests from
four suites, covering this AI fixture, ordinary and toroidal spell selection,
hero movement, and topology. Logs are `ai-town-before-test.log`,
`ai-town-after-build.log` and `ai-town-after-tests.log` on the TB5 build root.

This original test checks AI planning, not execution of the resulting cast in
a live game. It did not cover occupied towns, ties, multiplayer, or other AI
profiles; the occupied-town case was checked later as described below.

## Occupied nearest town

`Nullkiller2_AIGateway.unskilledTownPortalDoesNotSkipOccupiedNearestTownAcrossSeam`
places an unskilled caster and two allied towns on a 36-wide map. The town
across the seam has a visiting hero and a garrison, while the interior town is
free. With bounded topology, the AI has a same-turn Town Portal route to the
interior town, and the actual server spell handler moves the caster there.
With toroidal topology, the occupied town across the seam is nearer: the AI
does not plan a same-turn portal to the farther free town, and the server cast
leaves the caster at the origin. The fixture restores the caster between the
two cases and shares an in-process state between planning and server handling;
it does not execute the AI's planned action over a network or through a GUI.

The test target built on macOS arm64 with existing linker warnings. Twelve
related AI and server Town Portal tests passed; results are
`runtime/ai-town-occupied-regression-2026-11-15.{log,xml}` on the TB5 build
root. This covers the default spell configuration, not modded occupied-town
policies, equal-distance ties, all allied visibility states or AI profiles.

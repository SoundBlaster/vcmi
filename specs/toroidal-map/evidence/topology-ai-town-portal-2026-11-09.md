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

This checks AI planning, not execution of the resulting cast in a live game.
It does not verify occupied towns, ties, multiplayer, or other AI profiles.

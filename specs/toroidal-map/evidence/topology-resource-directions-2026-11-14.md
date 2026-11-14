# Resource interactions across edges and corners

`HeroMovementTest.HeroCollectsResourcesAcrossEveryToroidalEdgeAndCorner`
places a hero and a 1,000-gold resource on opposite sides of each edge and
corner of a synthetic 36×36 map, on both levels. For each of 16 combinations,
the pathfinder selects the wrapped adjacent target. The first server movement
request collects exactly 1,000 gold, removes the resource and leaves the hero
on its original tile. A second request enters the vacated tile without
collecting more gold. The fixture repositions the hero and creates one new
resource between cases; these are test setup operations, not gameplay steps.

Both requests consume movement points: 100 each for an orthogonal step and
141 each for a diagonal step in this fixture. A bounded-map control on this
branch shows the same behavior for an ordinary horizontal resource visit.
Source inspection of baseline `30e9207276f137658da5132705c05644bfe33aee`
shows the same ordering of cost calculation and blocking visit; an unmodified
binary or the original Heroes III game was not run for this comparison.
This result does not establish whether the two-request cost matches the
original game or a complete GUI route; it records current behavior rather
than deciding that policy.

The two resource and three road tests passed on macOS arm64 from `dc58aa0`
plus this test change. Other resources, protected objects, network clients,
GUI route execution and older game binaries remain unverified here.

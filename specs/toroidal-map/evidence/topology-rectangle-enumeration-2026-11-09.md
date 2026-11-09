# Shared rectangular map enumeration

`MapTopology::forEachTileInRectangle` enumerates the bounded or wrapped
rectangle around a valid center, once per canonical tile and on the same
level. It refuses negative ranges and invalid levels; callers must filter
sentinel positions before wrapping. Nullkiller2 now uses this common method
both for Dimension Door pathfinding and for its exploration and continuation
candidate scans.

The topology test compares the resulting sets with independently calculated
expected sets for map dimensions 3–7, asymmetric ranges 0–8 and both
topologies, and checks that no tile is visited twice. The exploration test
encloses a hero at `(0,10,0)` with only one other land tile at `(30,10,0)`:
it checks that a Dimension Door exploration goal to that tile exists on the
toroidal map and not on the bounded map.

The focused run passed 34 tests from six suites
(`topology-rectangle-final-build.log` and `topology-rectangle-final-tests.log`
on TB5). The standalone topology binary passed nine tests before the final
exploration-test-only edit (`topology-rectangle-standalone.log`). These tests
do not establish a live AI cast or every possible object/terrain interaction.

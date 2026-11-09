# Nullkiller2 cluster center on a torus

On toroidal maps, `ObjectCluster::calculateCenter` now uses a weighted circular
mean on each horizontal axis, then chooses the actual cluster object nearest
to that position using topology-aware distance. This avoids averaging objects
near opposite edges into the middle of the map. It retains the existing fourth
power of object priority and horizontal-only comparison across levels. Bounded
maps retain the original calculation. A zero-resultant circular mean is
ambiguous; the coordinate of the lowest-ID object is used as a deterministic
fallback for that axis.

`Nullkiller2_ObjectClusterCenter.clusterCenterTreatsOppositeEdgesAsNearby`
creates three actual game-state resources at x=0, 35 and 18 on a 36-wide map.
It verifies that the bounded result is x=18 and the toroidal result is an
edge object, then checks priority weighting and the fallback for equally
weighted opposite points. The edge assertion failed before the change and
passed afterward. The focused run passed 27 tests from five suites; logs on
TB5 are `cluster-center-before-test.log`, `cluster-center-after-build.log`
and `cluster-center-after-tests.log`.

This verifies the representative selected for one synthetic cluster, not the
quality of all AI strategic decisions or gameplay across levels.

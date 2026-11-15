# Nullkiller cluster blocker lifetime

`AIGateway::objectRemoved` informs the clusterizer before an object is
deleted. The clusterizer removes the corresponding blocked cluster from its
index, but a previously constructed `UnlockCluster` goal retains the cluster
through a `shared_ptr`. The old cluster held a raw pointer to the blocker, and
the goal's `toString()` dereferenced it. This identifies a lifetime hazard in
the code; no crash in an actual AI turn has been demonstrated.

The cluster now stores the blocker ID. `ClusterBehavior` resolves it through
the player-specific callback and skips the cluster when the blocker is no
longer accessible. An `UnlockCluster` goal snapshots the blocker's name and
position when constructed so that its description does not dereference the
blocker later. Normal object-removal notifications and cluster invalidation
remain in place.

`Nullkiller2_ObjectClusterCenter.resolvesBlockerAfterVisibilityChangesAndRemoval`
creates a goal with a real map resource at a seam, hides and reveals it, then
erases and releases the object while retaining the goal and cluster. It
checks callback resolution and the goal description in bounded and toroidal
modes. The first fixture version hid only one tile; on a toroidal map another
visible tile still exposed the object, so the fixture was corrected to hide
the map before selectively revealing the target. Nine focused cluster and
analyzer tests passed after correction; the result is
`runtime/ai-cluster-blocker-final-2026-11-15.xml` on the TB5 build root.
The client and test targets built on macOS arm64, with existing linker
warnings. The full enabled suite passed 1,729 tests, with three disabled;
results are `runtime/full-suite-cluster-blocker-2026-11-15.{log,xml}`.

This is a synthetic lifetime check, not a played AI turn, a concurrency test,
or a full audit of `AIPath`, priority evaluation, and object-ID replacement.
The ID lookup relies on the game's existing object lifecycle and locking
rules; it is not an independent lifetime or synchronization mechanism.

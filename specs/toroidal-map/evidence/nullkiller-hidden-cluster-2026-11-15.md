# Nullkiller cluster center with hidden targets

An object ID can remain in a cluster while the player-specific callback no
longer exposes that object. In a synthetic fixture with two resource targets,
`Nullkiller2_ObjectClusterCenter.ignoresTargetsNoLongerVisibleToPlayer` hides
one target after the cluster is assembled. Before the fix, selecting the
center raised `Cannot get object with id 1` instead of selecting the visible
target. The test also hides both targets and expects no center. Both bounded
and toroidal map modes are exercised.

The center calculation now collects the targets available through
`tryGetObj`, preserves the existing weighted selection for those candidates,
and returns `nullptr` if none are visible. The existing cluster behavior
handles a missing center. This change does not remove stale IDs from the
cluster or establish that other cluster accessors handle stale objects. The
fixture does not reproduce an actual AI turn, a race, or a removed object; the
frequency of the described situation in normal play is unknown.

On macOS arm64, the `vcmitest` and `vcmiclient` targets built successfully.
Eight focused cluster and analyzer tests passed; the full enabled suite passed
1,728 tests from 239 suites, with three tests disabled. The first full run
used the wrong external prototype-save fixture, causing its compatibility test
to fail; the complete run was repeated with the expected original untagged
save. Logs and XML results are under the checkout's TB5 build root in
`runtime/ai-hidden-cluster-before-2026-11-15.xml`,
`runtime/ai-hidden-cluster-after-2026-11-15.xml`, and
`runtime/full-suite-hidden-cluster-final-2026-11-15.{log,xml}`.

## Enumerating cluster targets

The same fixture subsequently exercised `getObjects`, which is used by
exploration, recruitment, startup and capture behaviors. Before its fix, the
method raised the same exception when one target was hidden. It now filters
unavailable IDs through the player callback. The fixture checks one visible
target, no visible targets, and a target becoming visible again, on both
bounded and toroidal maps. The cluster's IDs are not removed by this lookup.
Eight focused tests passed; the before and after XML results are
`runtime/ai-hidden-list-before-2026-11-15.xml` and
`runtime/ai-hidden-list-final-2026-11-15.xml` under the TB5 build root.
The full enabled suite passed 1,728 tests, with three disabled; its results are
`runtime/full-suite-hidden-list-2026-11-15.{log,xml}`.
This synthetic test does not establish that a real AI turn encounters such a
visibility change. Blocker lifetime was addressed separately in the
[blocker check](nullkiller-cluster-blocker-2026-11-15.md).

# Nullkiller empty blocked cluster

`ObjectClusterizer::validateObjects` checks whether each blocked cluster is
empty before scanning it for removed objects. A later `onObjectRemoved` in
the same validation pass can remove the final member. If no invalidated IDs
remain, `clusterize` can return with that cluster still in `blockedObjects`.
`ClusterBehavior` previously asked it for a center without checking for an
empty collection; on a toroidal map `calculateCenter` dereferenced a null
fallback, and on a bounded map it dereferenced the minimum of an empty range.
This is a source-derived failure path, not a crash observed during a game.

`calculateCenter` now returns `nullptr` for an empty cluster, and the only
caller skips that cluster. Existing nonempty cluster selection is unchanged.
The object-center test constructs a populated cluster, clears its targets,
and checks the empty result with bounded and toroidal map headers. The test
does not simulate the validation race, call `ClusterBehavior` with an empty
locked cluster, or play out the removal sequence.

On macOS arm64, the client and `vcmitest` targets built and 32 focused
Nullkiller object-center, object-clusterizer and one-way-portal behavior
tests passed. Logs on TB5: `runtime/ai-empty-cluster-build-2026-11-15.log`
and `runtime/ai-empty-cluster-tests-2026-11-15.log`. Existing duplicate
library and missing Conan search-path warnings appeared at link time. These
checks do not establish general Nullkiller correctness or network gameplay.

# Fork PR review: working assessment

This is a local, source- and evidence-scoped assessment of the review on
[SoundBlaster/vcmi PR #1](https://github.com/SoundBlaster/vcmi/pull/1), not a
claim that GitHub threads were answered or resolved. The user explicitly
allowed changes to legacy behavior when needed for the feature and requested
that each review comment be judged on its merits. Recheck the live PR and its
head before making any later review or merge decision.

| Review item | Assessment and branch evidence | Remaining question |
| --- | --- | --- |
| F1: turn can stall in AIR/WATER | Confirmed by [normal fog exploration](evidence/topology-stranded-fog-reproduction-2026-11-14.md). Fixed the timer gate and added [conditional return](evidence/topology-transient-origin-return-2026-11-14.md), [group prevalidation](evidence/topology-multiple-transient-return-2026-11-14.md) and a [disk-save check](evidence/topology-return-origin-disk-save-2026-11-14.md). | No general liveness guarantee for old saves without an origin, unavailable origins, vehicles and other cases. G12 remains pending. |
| F2: topology and movement policy are coupled | The behavior differences exist. Some validation was introduced to keep transient movement and server authority coherent on experimental toroidal maps. | Whether each rule should also apply to bounded maps and how to factor it needs individual review. A separate upstream PR is not part of the user's current delivery request. |
| F3: request acknowledgement changes globally | The cross-map compatibility concern is valid. The original server sent `false` when activities blocked a request but sent `true` after other visitor failures. The branch sends the visitor result. [Acknowledgement analysis](evidence/topology-acknowledgement-review-2026-11-14.md) explains that the result is not transactional. | Audit other consumers and partial operations; do not interpret a negative result as proof of no effects. |
| F4: branch-local format and capability tags | The tags are explicitly experimental and protect against silent interpretation of incompatible layouts; they are not upstream allocations. The compatibility and future-version concern is valid. | A future upstream-compatible allocation requires coordination and a migration decision for existing experimental saves. The user currently requests the feature branch in their fork only. |
| F5: repeated full pathfinding for transient landing | Potential server performance issue. The existing 252×252 synthetic check is a limited test, not a worst-case profile. | Measure realistic and adversarial movement, fog, and simultaneous-player workloads before choosing a cache or bound. |
| F6: changes outside topology | The diff contains general fixes and diagnostics. The user permitted legacy fixes that help the feature and requested the observation layer for testing. | Assess correctness, maintenance and runtime cost of each change independently; do not assume all belong in an upstream patch. |
| F8: hole lookup scans raw positions | The performance observation was confirmed in a bounded [microbenchmark](evidence/topology-hole-index-review-2026-11-14.md); the common path now scans only outside anchors. | Profile a real game if user-visible performance matters. Distinct raw holes can wrap onto the same tile, so a plain canonical set alone is insufficient without overlap accounting. |
| G1: spell preview and fog behavior | The cross-map visibility issue was addressed with [bounded and toroidal tests](evidence/topology-spell-fog-review-2026-11-14.md). | The suggested removal of the guard-visibility filter was not adopted: server and player callbacks can expose different guard sets, so preserve the existing information boundary. Other spell and information-leak paths remain to audit. |
| G2: redundant canonicalization and getter cost | Potential cleanup and performance concern, not established by the review alone. | Verify call-site invariants and measure before changing legacy-coordinate, object-position or mod behavior. |

No conclusion here is an acceptance decision for the feature. See the
[implementation matrix](implementation-matrix.md) for requirement-by-requirement
status and evidence, including unsupported or unverified profiles.

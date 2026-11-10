# Ranged adventure spells and AI landing candidates

Two later SoD tests invoke the real server Dimension Door spell handler on
revealed grass. From `(0,10)` to `(35,10)` and from `(0,0)` to `(35,35)`,
the bounded map rejects the out-of-range cast without spending mana; the
toroidal map moves the hero to the destination and spends mana and movement
points. Both tests passed on macOS arm64. They do not verify the client cursor,
animation, hidden destinations, guards, other terrain or other spells.

The common ranged-adventure-spell check now compares shortest wrapped x/y
displacements on toroidal maps. It continues to check whether source and
target are on the map, retains existing visibility checks, and preserves the
previous horizontal-only treatment of levels. This common code is used by
Dimension Door and Remove Object effects.

Nullkiller2's Dimension Door pathfinder now enumerates wrapped intervals of
candidate x and y coordinates instead of clipping them at the map boundary.
Intervals that span an entire axis enumerate each tile only once. Existing
landing, guard, cost and spell validation still run for each candidate.

`Spells_AdventureSpellRangedEffect.rangeWrapsAcrossToroidalEdgesAndCorners`
checks a seam, a corner, an out-of-range destination and bounded behavior.
`Nullkiller2_AIGateway.plansDimensionDoorLandingAcrossToroidalSeam` encloses a
hero and checks a same-turn Dimension Door plan to `(30,10,0)` from
`(0,10,0)` on a 36-wide map: the plan exists on the torus but not on the
bounded map. Each toroidal case failed before its respective change and
passed afterward. The focused run passed 51 tests from eight suites; logs
on TB5 include `ranged-before-test.log`, `ai-dd-before-test.log`,
`ai-dd-after-build.log` and `ai-dd-after-tests.log`.

In a subsequent change, Nullkiller2's separate cluster reach heuristic also
uses wrapped displacement. The focused run in `ai-dd-reach-tests.log` passed
29 tests, including a new edge/corner case with bounded, out-of-range and
different-level comparisons. This tests the heuristic, not an assembled
cluster-selection decision.

The later [rectangle-enumeration change](topology-rectangle-enumeration-2026-11-09.md)
also adapts the AI's separate exploration and continuation scans.

These checks do not prove the behavior of every ranged spell. Cross-level
spell semantics, animation, visibility and multiplayer require further
verification.

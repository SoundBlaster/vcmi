# Shared horizontal distance

`MapTopology::horizontalDistanceSquaredIgnoringLevel` centralizes the wrapped
2D distance used by nearest-town spell selection, Nullkiller2 Town Portal
planning, subterranean gate pairing, and Grail radius selection. Its name
makes the legacy cross-level comparison explicit. It returns no value when
the source level or a bounded coordinate is invalid. Callers are responsible
for filtering sentinels: wrapping an unspecified position would give it a
meaning it does not have.

The new topology test compares opposite corners on toroidal and bounded maps,
cross-level inputs, an out-of-bounds bounded position, and an invalid source
level. After the refactor, the focused run passed 40 tests from five suites
and the standalone topology binary passed eight tests. Build and test logs
on TB5 are `horizontal-distance-build.log`, `horizontal-distance-tests.log`
and `horizontal-distance-standalone.log`.

Before this refactor, the full `vcmitest` suite at commit `013514d` passed
1,342 tests from 219 suites; three tests were disabled
(`full-suite-after-topology-audit.log`). That full-suite result predates the
refactor and is not evidence of a full post-refactor run.

After the refactor and the later ranged-spell and AI-reach changes, the full
`vcmitest` suite at code commit `f512e47` passed 1,346 tests from 219 suites;
three tests were disabled (`full-suite-wrapped-spells.log`). The later
`30c436e` commit changes documentation only. This local run does not establish
visual continuity or multiplayer behavior.

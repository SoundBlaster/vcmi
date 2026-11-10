# Position-targeted objectives across a seam

`CMap::checkForObjectives` previously checked a condition's raw position
against rectangular map bounds. A control objective targeting a town at
`(-1,-1,0)` remained unresolved, even though the town was present through
both seams. `getObjectiveObjectFrom` also indexed its input directly, which
was unsafe for an invalid transport position.

Objective lookup now canonicalizes valid-level coordinates before looking
up visitable cells, and compares fallback object anchors by canonical
position. Bounded maps continue to reject outside coordinates. Invalid
levels, including `(-1,-1,-1)`, do not wrap into real tiles.

`HeroMovementTest.ObjectiveResolvesTownAnchorAcrossToroidalSeams` failed on
the control case before the fix. Afterward it passed for CONTROL,
CONTROL_CURRENT, DESTROY, HAVE_BUILDING and TRANSPORT, using both raw and
canonical anchors and the visitable cell, plus invalid-level and bounded-map
checks. The separate wrapped-main-town startup test also passed. The macOS
arm64 client, server, editor and test targets built; the full suite passed
1,441 tests, skipped three optional external-input tests and reported five
disabled tests (`full-suite-wrapped-objectives.log` on TB5). Existing linker
warnings remain.

These tests cover resolution to a town object ID in memory, not complete
victory or defeat execution, all object types, scenario-condition updates
after editor moves, or ambiguous overlapping targets. An unresolved transport
objective is not made a supported playable condition by this change.

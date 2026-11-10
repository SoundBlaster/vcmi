# Starting heroes at a wrapped main town

`ToroidalDemoVmapTest.StartingHeroUsesMainTownOutsideMap` creates a synthetic
SoD town, moves its anchor to four raw coordinates beyond edges and corners,
marks it as the main town, and saves a toroidal VMAP in memory. The test
reloads the header, checks that the raw main-town coordinate and generation
setting survive, then starts an actual game state. For each anchor it runs
once with a generated hero and once with a hero already at the entrance: in
both cases exactly one owned hero ends up visiting the town at its canonical
entrance. All eight combinations passed on the macOS arm64 build. The related
town VMAP round-trip test passed as well.

This check corrected an initial assumption: `int3::isValid()` checks the level
coordinate, not whether x or y is negative. No format change was needed for
these cases. They do not establish campaign behavior, every town appearance,
interactive object overlap or visual presentation in the editor or game.

## Moving the main town in the editor

The original test moved the town directly and set its player metadata
manually. Replacing that setup with `CMapEditManager::moveObject` exposed a
separate defect: the town moved, but `posOfMainTown` retained the old anchor.
The resulting map could generate its hero at the wrong position.

The editor operation now records player main-town references that match a
town's original anchor and updates them with the move. Undo and redo update
the same references only while they still point at the expected position, so
an intervening user selection is not overwritten. Non-town moves do not
change main-town metadata. This applies to bounded and toroidal maps.

The existing VMAP/startup test now uses the editor operation and verifies
move, undo, redo and all eight wrapped-town startup combinations. A separate
bounded test checks a non-town move and a user-reassigned main town before
undo. Three focused tests passed. The macOS arm64 client, server, editor and
test targets built; the full suite passed 1,440 tests, skipped three optional
external-input tests and reported five disabled tests
(`full-suite-editor-main-town-move.log` on TB5). Existing linker warnings
remain. References for other scenario conditions have not been verified by
this change, and no live editor UI check was obtained.

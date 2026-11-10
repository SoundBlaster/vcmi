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

# Legacy bounded seam regression

`HeroMovementTest.LegacyBoundedMapsRejectDirectSeamStepsOnBothLevels` starts a
generated two-level SoD H3M and checks that it defaults to bounded topology.
For all eight outward edge and corner directions on both levels, it positions
the hero at a seam and compares a direct move to the opposite side. In bounded
mode, the pathfinder must not use the opposite side as a direct step and the
server must reject it without spending movement points or moving the hero.
With toroidal mode explicitly enabled, the same step is direct, the server
accepts it, and its movement points agree with the pathfinder.

This test and three related topology and fog tests passed three consecutive
runs on macOS arm64. It is a synthetic H3M fixture and changes the topology
in memory for the positive control. It does not establish every legacy map,
save/load, object interaction, or graphical behavior.

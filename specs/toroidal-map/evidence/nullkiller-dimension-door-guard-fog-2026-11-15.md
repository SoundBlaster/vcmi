# Nullkiller Dimension Door landing and hidden guard

`Nullkiller2_AIGateway.dimensionDoorSeamPlanningUsesVisibleGuardInformation`
tests a generated 36×36 toroidal map with Dimension Door configured to
trigger guards. A weak hero is enclosed by rock and can cast to a destination
on the west edge. A strong monster on the east edge guards that destination
across the seam. The test controls the player's fog directly and recomputes
Nullkiller's paths after every change.

The entirely hidden destination has no planned cast, both with and without
the monster. With the destination visible but the guard hidden, the test
confirms that the unrestricted guard query sees the monster while the
player-facing guard list does not. A same-turn Dimension Door plan exists;
after removing the hidden monster, its movement cost, path danger and army
loss match the plan with the monster present. Revealing the overwhelming
guard prevents that plan. This compares the first matching cast plan, not
every candidate path.

The hidden-destination result is consistent with the path node's
accessibility rejecting fogged cells. `getDimensionDoorLandingInfo` still
calls `isTileGuardedUnchecked` before that rejection, so this test does not
prove that every information path, timing behavior or visible-object
combination is independent of hidden guards. It also does not execute the
spell or a full AI turn. No production change is justified by this result
alone.

On macOS arm64, the Release `vcmitest` target built from `afb1498` plus this
test, and the new test passed with the existing seam-planning test and four
Dimension Door exploration tests (six total). Results are in
`runtime/ai-dd-fog-guard-2026-11-15.xml` under the TB5 build root. The linker
reported duplicate libraries and absent Conan search paths, but completed.

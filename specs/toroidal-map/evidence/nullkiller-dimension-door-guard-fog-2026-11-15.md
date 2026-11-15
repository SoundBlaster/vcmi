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
accessibility rejecting fogged cells. At `afb1498`,
`getDimensionDoorLandingInfo` called `isTileGuardedUnchecked` before that
rejection. The empty-landing comparison did not cover visible objects.

On macOS arm64, the Release `vcmitest` target built from `afb1498` plus this
test, and the new test passed with the existing seam-planning test and four
Dimension Door exploration tests (six total). Results are in
`runtime/ai-dd-fog-guard-2026-11-15.xml` under the TB5 build root. The linker
reported duplicate libraries and absent Conan search paths, but completed.

## Visible armed object and fix

`Nullkiller2_AIGateway.hiddenSeamGuardDoesNotChangeDimensionDoorPlanToVisibleArmedObject`
adds an artificial, visible, armed garrison whose synthetic footprint is
visitable but nonblocking. A legacy H3M mask can express this footprint; the
fixture does not establish its prevalence in real maps. With a hidden guard,
the old implementation called the danger evaluator because of the unchecked
guard query; that evaluator also assessed the visible armed object. The weak
hero consequently had no cast plan. After removing the unseen monster, the
same landing did have a cast plan. The new test failed on that comparison
before the fix (`runtime/ai-dd-armed-hidden-before-2026-11-15.xml`).

For toroidal maps, landing planning now checks the player's known destination
and visible guard list before evaluating danger. It avoids the unrestricted
guard query in this path. Bounded maps retain their previous behavior. The
test now compares the first cast plan's cost, danger and army loss with the
monster hidden and removed; revealing the overwhelming guard prevents the
plan. It does not execute the cast, validate the modded object's interaction,
or establish independence of every possible AI information path.

After the fix, `vcmitest`, Nullkiller2 and `vcmiclient` built, and 169 matching
Nullkiller and Dimension Door exploration tests passed on macOS arm64; see
`runtime/ai-dd-armed-hidden-related-2026-11-15.xml` on TB5. The linker again
reported duplicate libraries and absent Conan search paths.

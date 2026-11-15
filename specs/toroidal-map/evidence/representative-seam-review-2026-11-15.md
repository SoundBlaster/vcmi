# Representative seam review

The movement and compatibility evidence linked from the execution checklist
was reviewed against its stated representative scope. On macOS arm64, the
existing TB5 `vcmitest` binary was rerun on 2026-11-15 with six selected
`HeroMovementTest` cases and the parameterized
`EdgeAndCornerOnBothLevels/ToroidalGuardedDisembarkTest` suite. All 54 tests
passed. The XML report is `runtime/representative-seam-review-2026-11-15.xml`
under the TB5 VCMI build root. The test used an isolated local data profile.

The selected cases cover blocked terrain, road costs and mixed transitions,
resource collection, embarking and disembarking, one unsafe transit route,
and guarded or visitable shores. Their detailed fixtures and limits are in
the original evidence records linked by the checklist. The executable was
already present; this run did not independently establish its exact source
revision and is not a clean rebuild of the current branch. It does not replace
the prior graphical, network or legacy-format checks and does not prove all
game mechanics or animation.

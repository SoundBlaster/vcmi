# Pathfinder options lifetime

`CPathfinderHelper` stored `PathfinderOptions` by reference. The authoritative
`CGameHandler::moveHero` constructed a helper from a temporary options object,
then used the helper after that temporary was destroyed. Two existing tests
also used that pattern. This was a dangling reference on ordinary as well as
toroidal maps; the exact impact depended on which options the helper read.

An experimental pathfinder view exposed the defect as a SIGBUS when movement
accessed an added option. The crash occurred with that experimental change,
not in a separately demonstrated unmodified upstream binary. Inspection of
the construction and member lifetime establishes the pre-existing defect.

The helper now owns an immutable snapshot of its options. Callers must finish
configuring options before constructing the helper; subsequent caller-side
changes do not alter an existing helper. `MovementCostTest.helperOwnsItsPathfinderOptions`
checks the ownership type, constructs a helper whose caller-local options go
out of scope, checks selected options and computes a movement cost.

On macOS arm64, `vcmitest`, `vcmiserver` and `vcmiclient` built successfully.
The full suite passed 1,475 tests from 226 suites without skips; four remain
disabled. The optional prototype save, VMAP and Complete legacy map were
supplied. The log is `full-suite-pathfinder-options-lifetime-2026-11-10.log`
in the TB5 runtime directory. This does not prove the entire movement or AI
implementation correct.

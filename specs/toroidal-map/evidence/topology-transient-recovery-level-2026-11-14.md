# Transient recovery respects the hero's current level

The emergency origin-return code previously accepted a recorded LAND
position on a different level from the stranded hero, provided the position
was otherwise safe. It could therefore return a hero from the underground to
the surface. Normal teleportation and ordinary landing clear the recorded
origin; this finding is demonstrated with a deliberately inconsistent state,
not a normal sequence of game actions.

`TransientRecoveryMustNotReturnHeroToAnotherLevel` first records a surface
origin through an ordinary Water Walk step, then synthetically moves the hero
to underground water while keeping the stale origin. Before the fix, recovery
moved the hero back to the surface and consumed its last movement point. The
server now rejects a recorded origin on another level. The new test checks
both the current handler and a handler restored from a memory-serialized
archive: neither relocates the hero or consumes its remaining point.

On the TB5 macOS build, `vcmiserver` and `vcmitest` built. The new test,
the existing same-level emergency return, atomic group recovery, and the
two-peer TCP lobby test all passed. The full suite ran 1,507 tests: 1,504
passed, three optional external-fixture tests were skipped, and four tests were
disabled. Its TB5 runtime log is
`full-suite-transient-recovery-level-2026-11-14.log`.

This is not an on-disk save/load or GUI test. Rejecting an invalid origin
preserves level isolation; it does not solve the remaining need to recover a
hero when no valid same-level destination is known.

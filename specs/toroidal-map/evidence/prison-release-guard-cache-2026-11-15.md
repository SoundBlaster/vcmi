# Guard cache after a hero is released

`GiveHero` replaces the imprisoned hero's appearance and reindexes its
footprint. The map's guard cache was not recomputed for either footprint. A
synthetic prisoner with a blocking, direction-restricted appearance at
`(0,5)` and a monster at `(35,5)` demonstrates the difference on a toroidal
map: after `giveHero` and the ID update, the map's direct guard calculation
returns the monster, while the cached game-state lookup still reports no
guard. The focused test failed on that final assertion before the fix.

`GameStatePackVisitor::visitGiveHero` now refreshes the guard neighborhoods
of both the old and new appearance after reindexing. The regression checks
the synthetic release across the seam and an equivalent interior arrangement
on a bounded map. This is an actual server `GiveHero` application, but the
fixture constructs the prisoner by changing an initialized hero rather than
visiting a real prison; it does not simulate a remote replica or GUI play.
The normal packet visitor is shared by game-state replicas, but their behavior
for this specific packet was not separately observed.

On macOS arm64, `vcmitest`, `vcmiclient`, and `vcmiserver` built; 49 focused
release, guard-cache, movement, disembarkation and activity tests passed.
Logs on TB5: `runtime/prison-guard-before-test-2026-11-15.log`,
`runtime/prison-guard-after-build-2026-11-15.log`,
`runtime/prison-guard-final-build-2026-11-15.log` and
`runtime/prison-guard-related-tests-2026-11-15.log`. The link step reported
existing duplicate-library and absent Conan search-path warnings. Other
appearance-changing packets, templates, map levels and guards remain outside
this test.

The full enabled local suite subsequently passed 1,670 tests from 235 suites;
four tests were disabled. The run supplied the earlier untagged prototype
VMAP and save and the Complete `Dead and Buried` map as optional compatibility
inputs. Their SHA-256 values matched before and after the run. Full log:
`runtime/full-suite-prison-guard-2026-11-15.log` on TB5. This local suite does
not replace an interactive prison visit or multiplayer verification.

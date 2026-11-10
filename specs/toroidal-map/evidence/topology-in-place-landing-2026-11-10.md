# Landing on the current tile after timer expiry

An airborne, on-foot hero can be above a clear land tile and still have a
confirmed `AIR` layer. The earlier timer recovery searched only for a path to
another tile. A new regression test reproduced a stranded hero with zero
movement points above the only remaining land tile.

On an expired toroidal turn timer, the existing recovery now first permits an
in-place `AIR` to `LAND` transition when the current terrain is passable land,
no other object blocks or can be visited on that cell, no creature guards it,
and the complete neighboring area is visible to the owner. It does not spend
movement points, move the hero, reveal terrain, or visit an object. The server
broadcasts an explicit same-position movement-layer update and persists the
new layer. Ordinary same-position movement packets with an unspecified layer
retain their prior behavior. If these conditions do not hold, the previous
known-route search is used; the timer may still wait indefinitely when neither
approach succeeds. This change does not implement general stranded-hero rescue.

`AutomaticLandingCanStopOnCurrentClearLand` verifies the expired-timer case,
unchanged position and points, rejection when a neighboring tile is hidden or
an event occupies the tile, packet rollback/reapplication, and server-state
serialization. The targeted automatic-landing and timer tests all pass. The
TCP loopback lobby test synthetically sets a hero to `AIR` over clear land,
then asks the actual server recovery to land it and checks two replicated
states retain position and movement points and receive `LAND`. It then advances
the turn. The initial airborne state in this particular TCP check is synthetic;
remote networking, wall-clock timer scheduling and graphical behavior for this
case remain unverified.

On macOS arm64, `vcmitest`, `vcmiclient` and `vcmiserver` built with
linker warnings about duplicate libraries and missing Conan search paths. The
full suite then passed 1,493 tests from 226 suites, with four disabled and no
skips; the branch-era prototype save, VMAP and Complete legacy map were
supplied. The log is `runtime/full-suite-in-place-landing-final-2026-11-10.log`
on TB5. `specpm validate . --json` was also run for the package.

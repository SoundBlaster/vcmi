# Timer recovery from aviation transit above an obstacle

An `AVIATE` vehicle can use `AIR` to cross a blocked tile, where it cannot
settle in place. The timer previously could be held by the movement gate and
the automatic recovery routine skipped vehicles; the new focused test failed
before this change.

For toroidal `AIR` transit in an `AVIATE` vehicle, the timer can now try a
single ordinary pathfinder step to an adjacent `AVIATE` tile. The step must be
reachable on the current turn, be free of visitable or blocking objects and
guards, have a known guard neighborhood, and reveal no additional terrain.
If no such step is available, the timer waits and retries. The vehicle and
ordinary movement cost are preserved. This is not a multi-step vehicle search
or a general guarantee of recovery.

While leaving `AIR` transit in such a vehicle, the server skips object-leave
callbacks at the source: a hero flying over those objects did not visit them.
The automatic step permits an event under the source without calling it.

`HeroMovementTest.ExpiredTimerMovesAviationVehicleOffBlockedTransitTile`
embarks a synthetic aviation vehicle, flies across a seam above a blocker, and
checks direct timer updates. With surrounding tiles hidden, then with only
one movement point, it waits in `AIR`; once the map and points permit the
step, it moves to `AVIATE`. An invisible source event receives no leave
callback. The test then synthetically restores the same transit position and
points with the event removed; automatic recovery chooses the same tile and
cost. This comparison covers that fixture only, not all hidden-information
channels. Visibility and movement-point changes and the second transit state
are synthetic.

The macOS arm64 TB5 server and tests built from `85ae321` plus this change.
Eight focused tests passed. The full local suite before the final paired-event
test addition passed 1,520 of 1,523 tests from 226 suites; three optional
fixture tests were skipped and four remain disabled. The log is
`runtime/full-suite-aviate-adjacent-2026-11-14.log`. The final paired-event
addition was rerun in the focused suite. Neither GUI play, an actual vehicle
mod, wall-clock timing nor remote clients were tested here.

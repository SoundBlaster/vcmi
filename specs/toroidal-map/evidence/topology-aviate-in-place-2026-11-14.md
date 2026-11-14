# In-place settlement for an aviation vehicle

On a toroidal map an `AVIATE` vehicle can use the `AIR` transit layer. The
end-turn guard treated this as transient, but in-place landing accepted only
heroes on foot. Thus even over a clear tile the player could not settle the
vehicle in place through that path.

The server can now settle an `AIR` hero in an `AVIATE` vehicle to `AVIATE`
on the same tile when the tile contains no blocking or visitable object other
than the hero and the adjacent visibility is known. The existing in-place
movement packet preserves position, movement points and vehicle identity.
Manual group settlement, group recovery and timer-driven settlement use the
same check. Other vehicle layers and recovery above an unsuitable tile are not
covered by this change.

`HeroMovementTest.AviationVehicleCanSettleInPlaceAfterAirTransit` creates a
boat and changes its layer to `AVIATE` for the fixture. A hero embarks, crosses
the seam in `AIR`, and is prevented from settling while a visitable event is
underneath or an adjacent tile is hidden. After those conditions are removed,
direct timer updates settle the hero to `AVIATE` without moving or losing the
vehicle. The test synthetically sets the layer back to `AIR` and also checks
manual in-place settlement. The test initially failed at in-place settlement
before the fix. A separate timer fixture without an opponent ended its game
before the timer check; the final fixture retains a second player.

On macOS arm64, `vcmiserver` and `vcmitest` built from `37a5c61` plus this
change. The focused six tests passed. The full local suite passed 1,518 of
1,521 tests from 226 suites; three optional external-fixture tests were skipped
and four tests remain disabled. The TB5 log is
`runtime/full-suite-aviate-in-place-2026-11-14.log`. Linker warnings reported
duplicate libraries and two missing search directories. This is not a GUI,
remote multiplayer, actual mod or wall-clock timer test; mixed group recovery
with such a vehicle also remains unverified.

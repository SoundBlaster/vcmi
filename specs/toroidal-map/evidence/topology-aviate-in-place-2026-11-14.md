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
same check. A later [adjacent-landing change](topology-aviate-adjacent-landing-2026-11-14.md)
adds a conditional ordinary step from an unsuitable tile. Other vehicle layers
and general recovery remain open.

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
remote multiplayer, actual mod or wall-clock timer test.

`HeroMovementTest.GroupRecoveryPrevalidatesAviationVehicleAndStrandedHero`
adds a mixed group: the aviation vehicle crosses a seam in `AIR`, while a
second hero is synthetically stranded on water with one movement point. When
the only suitable land is hidden, group recovery leaves both heroes in place
and the vehicle in `AIR`. After the tile is made visible, recovery settles
the vehicle in `AVIATE` and moves the other hero to land. The vehicle identity
is preserved. This and two related group and aviation tests passed on macOS
arm64 using `af6b9c4` plus the added test. The visibility change and stranded
state are synthetic; this does not establish packet ordering on remote clients.

## Guard and terrain changes under the vehicle

The in-place check previously accepted a vehicle over an impassable tile or
a tile guarded by a creature, unlike the ordinary adjacent recovery search.
`HeroMovementTest.AviationInPlaceLandingRejectsGuardAndImpassableTerrain`
embarks in a synthetic `AVIATE` vehicle, flies across the seam, and then
spawns a guard across that seam adjacent to the vehicle. Before the fix,
in-place settlement incorrectly succeeded. The test also changes the tile
under the airborne vehicle to rock and observed the same incorrect success.
Both changes are synthetic ways to create changed-world states.

In-place settlement now requires passable terrain and no guarding creature,
in addition to the existing occupancy and revealed-neighborhood checks. The
test verifies that both in-place settlement and group recovery leave the
vehicle airborne while either hazard is present. After removing the hazard,
settlement succeeds without moving the hero or changing points or vehicle.
The guard placement uses the server object-creation API; the terrain change
is a direct test-state mutation. This does not add an encounter or resolve
recovery when no safe tile exists.

On macOS arm64, `vcmiserver` and `vcmitest` built from `ef25941` plus the
change. All 32 selected aviation, transient movement, landing, recovery and
TCP-lobby tests passed; the report is
`runtime/aviate-in-place-hazards-final-2026-11-15.xml` on the TB5 build root.
The linker reported duplicate libraries and missing Conan search paths. The
new hazard cases themselves are in-process, not GUI or remote multiplayer.

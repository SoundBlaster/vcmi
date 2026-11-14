# Multi-step aviation recovery

The previous automatic aviation recovery could take only one step to a clear
`AVIATE` tile. It could not recover a vehicle surrounded by several blocked
tiles even when a known hover tile was reachable within the turn.

The server now searches same-turn pathfinder routes ending on an unoccupied,
unguarded `AVIATE` tile. Teleport routes are disabled; preceding route steps
must remain in `AIR`, use ordinary actions, retain movement points, pass
through known neighborhoods and reveal no additional terrain. The server
executes one step and searches again on the next timer update, allowing world
and visibility changes to stop the recovery. It does not guarantee a result
when no qualifying route exists.

The expanded `HeroMovementTest.ExpiredTimerMovesAviationVehicleOffBlockedTransitTile`
first exercises the adjacent and paired invisible-source-event cases. It then
synthetically restores the `AIR` state and adds a 3×3 blocked footprint around
the vehicle. Timer updates move the vehicle one step in `AIR`, wait when a
neighbor is synthetically hidden, and settle it in `AVIATE` after the tile is
revealed again. The vehicle identity remains unchanged. Before the change,
the two-step route was not found. Eight focused tests passed on macOS arm64
from `511c8d3` plus the implementation and test change.

The local full suite passed 1,520 of 1,523 tests from 226 suites; three tests
requiring optional external fixtures were skipped and four remain disabled.
The TB5 log is `runtime/full-suite-aviate-route-2026-11-14.log`.

This is a synthetic map and directly driven timer, not GUI or remote-network
evidence. The tests do not prove confidentiality for every hidden object,
multi-hero interaction, timing or recovery after arbitrary world changes.

`HeroMovementTest.AviationRecoveryFindsKnownRouteOnLargeToroidalMap` uses a
252×252 synthetic map: an aviation vehicle crosses both seams at a corner,
enters `AIR` above a 3×3 blocker and settles in `AVIATE` after two direct
automatic recovery calls. This test and the 36×36 timer case passed on macOS
arm64 from `71e242c` plus the added test. The large case took 38 ms in this
run; this is a single local observation, not a general performance benchmark.

# Full-map fog range

The `FULL_MAP_RANGE` branch of `getTilesInRange` ignored the requested
visibility state and returned every cell. It also called `getTile` on a
player-specific callback, which throws for a hidden tile. The finite-radius
branch already filters by the selected team's fog without reading hidden
terrain. The full-map branch now does the same while retaining its existing
all-level scope; a request without a player still returns every cell.

Before the fix, the new range test returned 2,592 cells instead of two
revealed cells and a player callback threw `CallbackInvalidCallException`.
After the fix, `HeroMovementTest.FullMapRangeFiltersFogForBothLevelsAndTopologies`
passes for bounded and toroidal maps, both levels, two teams, and unrestricted
and player-specific callbacks. `HeroMovementTest.FullMapFogChangesKeepOwnedSightAndOtherTeamIsolation`
exercises the server's full-map reveal and hide, checking the hero's retained
sight and the other team's unchanged fog. Both tests passed three consecutive
runs on macOS arm64. These tests do not establish confidentiality against a
malicious multiplayer client or graphical fog rendering.

The full `vcmitest` run on TB5 passed 1,536 tests; three tests requiring
explicit prototype-save, prototype-map and legacy-H3M fixture paths were
skipped, and four tests were disabled. The log is
`runtime/full-suite-full-map-fog-2026-11-14.log`. SpecPM validation reported
no errors or warnings.

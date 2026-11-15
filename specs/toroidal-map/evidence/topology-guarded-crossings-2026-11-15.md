# Entering guarded cells across map seams

`ToroidalGuardedCrossingTest.EnteringGuardedTileStartsBattleWithCorrectMonster`
places a hero on an edge or corner and an empty destination across the seam.
A monster one further step away guards the destination but not the origin.
On synthetic, fully revealed 36×36 maps, the test checks the map's cached
guard position, executes the actual server movement request and verifies that
the battle's defender is that monster. It covers all eight directions on
both surface and underground: 16 cases passed on macOS arm64. The XML report
is `runtime/guarded-crossing-directions-2026-11-15.xml` on the TB5 VCMI build
root. This expands the earlier edge and corner guard checks to all directions
and levels; it does not establish full battle resolution, GUI animation,
fog behavior, other guard arrangements or pathfinder behavior.

The test binary was built from `814fdbf` plus this fixture. The build
succeeded with duplicate-library and missing Conan search-path warnings.
The complete local suite passed 1,725 tests with three disabled, with the
prototype save, VMAP and legacy H3M inputs supplied. The log and XML report
are `runtime/full-suite-guarded-crossing-2026-11-15.{log,xml}` under the
same build root. SpecPM validation passed without warnings.

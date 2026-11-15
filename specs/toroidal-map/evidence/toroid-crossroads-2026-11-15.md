# Toroid Crossroads demonstration scenario

`Toroid Crossroads` is a separate 36×36 single-level, two-player map. Each
player starts with a Castle town and hero; the scenario has six resource
piles, four neutral wood/ore mines and two neutral monster stacks. Four dirt
roads form two horizontal and two vertical loops. One road on each axis
crosses its respective seam, and dirt patches cross the east-west seam. The
two sides have matching counts of nearby resources and mines, but their heroes
have different abilities and the scenario has not been balanced by playtesting.
The sparse `Toroid Seam Demo` fixture remains unchanged.

The versioned map is [`../demo/toroid-crossroads.vmap`](../demo/toroid-crossroads.vmap),
SHA-256 `dc22f9087595505366d7e6c6ff585df4e9b265436a18a2c7d6d539493848735f`.
It is also installed in the local VCMI Maps directory on TB5. The archive
contains four JSON files and no original game artwork. It requires this
experimental engine and the player's own game resources.

`MapFormat.ToroidalCrossroadsScenarioRoundTrips` builds the map, paints the
roads and terrain using the editor's operations, and saves and reopens the
VMAP. It verifies the topology flag, the object counts, absence of overlapping
visitable cells and static seam-spanning footprints, road tile types and
selected terrain tiles. It checks header and object JSON for test-only mod
references. `ToroidalDemoGameTest.CrossroadsMinesAreReachableByBothPlayers`
initializes the H3M-derived object layout, enables the topology, reveals the
map to each player and checks a pathfinder route to each mine. That test uses
the object layout before roads and dirt are painted, and full visibility; it
does not execute the routes or validate fog-of-war exploration. The new tests
and original sparse fixture passed (`runtime/crossroads-validation-2026-11-15.xml`
on the TB5 build root).

The complete local macOS arm64 test run passed 1,975 tests, skipped five
external-fixture or external-server tests and retained three disabled tests.
The log and XML are `runtime/full-suite-crossroads-2026-11-15.{log,xml}` on
the TB5 build root. SpecPM package validation also passed without warnings.

The directly launched signed client at `ab35306` started an earlier generated
copy of this map. The red hero collected 1,000 gold across the northwest
corner, then moved `(0,0,0) → (35,35,0)` with movement `1560 → 1419 → 1278`.
One screenshot showed the roads meeting at the hero's starting tile and a
nearby town and sawmill. After red ended the turn, Nullkiller2 moved its
heroes and ended its turn; control returned to red. The game created an
autosave and exited normally. This is one local turn, not a complete match,
AI acceptance or an animation-continuity test.

The installed and tracked artifact was generated after the graphical run to
remove a duplicate warning from the scenario description. A comparison of the
two archives found `header.json`, `objects.json` and `surface_terrain.json`
byte-identical; the only changed field in `russian.json` was the description.
The graphical run therefore predates that text-only change. Its client log
and observation stream are in `runtime/gui-crossroads-2026-11-15/` on TB5;
the screenshot was observed but not retained. The ordinary Launch Services
startup issue is documented separately in the
[current preview record](topology-preview-ab35306-2026-11-15.md).

# Summon Boat across map seams

Summon Boat ranks available boats by Manhattan distance, ignoring level, after
giving priority to the caster's previously used boat. The distance now uses
the shortest wrapped displacement on a toroidal map; candidate filtering,
priority and tie behavior remain unchanged. The common boat generator also
canonicalizes an adjacent target before checking its terrain and occupancy.
Bounded maps retain their former behavior.

Two tests use a real game state and server-side spell cast on a synthetic
36×36 map. The first checks that bounded mode selects the interior boat,
whereas toroidal mode selects a boat across the seam. The second checks that
water across the seam is unavailable in bounded mode, then casts the spell
and verifies the boat appears on that canonical water tile in toroidal mode.
Both tests failed before their respective fixes. The final focused run passed
24 tests across Summon Boat, movement and Town Portal suites.

TB5 logs: `summon-boat-before-tests.log`, `summon-boat-final-build.log`,
`summon-boat-final-tests.log`. At commit `b09b1e5`, the full suite passed
1,353 tests with three disabled (`full-suite-summon-boat.log`).

The subsequent `shipyardBuildsBoatAcrossToroidalSeam` test loads an H3M
fixture with a real shipyard, switches the in-memory topology, and invokes
`CGameHandler::buildBoat`. It checks that bounded mode cannot use the water
tile across the seam and that toroidal mode builds a boat there. All three
Summon Boat and shipyard cases passed (`shipyard-seam-tests.log`). This is a
synthetic in-memory topology switch; it does not establish that the fixture
passes the current authored-map restriction on static objects.

Two additional server-side cast tests cover level and selection behavior. In
the first, bounded mode chooses a surface boat while toroidal mode chooses a
closer boat across the seam on the underground level. In the second, two
boats have equal wrapped distance and the later candidate wins; after marking
a farther boat as last used by the caster, the farther boat wins instead.
All eight `SummonBoatTopologyTest` cases passed three consecutive runs on
macOS arm64. GUI effects, other level arrangements and multiplayer remain
unverified here.

## Nullkiller planning from an edge shipyard

`Nullkiller2_Behaviors_WaterExploration.plansBoatFromShipyardAcrossToroidalSeam`
loads a synthetic 36×36 map with a hero and owned shipyard near the western
edge, water at the eastern edge and one hidden sea cell. The shipyard cannot
build on that distant water in bounded mode. After the in-memory topology
switch, its boat location is `(35,10,0)`; Nullkiller produces an unblocked
exploration route containing a `BuildBoatAction` at that exact tile and ending
on water. The four water-exploration tests and eight `SummonBoatTopologyTest`
server tests passed three times each on macOS arm64 with build `0e04dce` plus
the test in `7f3dc0b`. This verifies AI route planning and separately tested
server construction; it does not execute that AI route, establish fog safety
for arbitrary hidden configurations, or validate GUI/network play.

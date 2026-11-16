# Earth draft: active-object overlap audit

The read-only `tools/audit_earth_overlaps.py` compares serialized active
footprint cells in the original two-level converted VMAP and the final
single-level toroidal Earth draft. It wraps draft coordinates and subtracts
object pairs that already overlap an interactive cell in the bounded source.
It reports pairs with at least one visitable `A` or `T` cell. This is a mask
comparison, not engine pathfinding or proof that a player is blocked.

For source SHA-256
`69e1f1aff0398a7042e7b158b1ac638ae3237b23d0a5a8927f0afcb7ef0c4023`
and demo SHA-256
`61f9f042612fb86aa611c64e57b2c2e6ee7a39409cf2b4eeb1fd74b8b00d40b7`,
the audit reports 161 new interactive pairs; 267 active source cells lie
outside the bounded source levels and are excluded from the source baseline.
The JSON is local on TB5 at `runtime/earth-overlap-audit-2026-11-15.json`,
SHA-256 `85c06b6835517b647a964b92710466af27b6582fd7020990ed3ba81b67ea2d79`.

One candidate is `creatureBank_10789` and `creatureBank_11365`. At `(126,243)`
the first has `B` and the second `A`; at `(127,243)` these roles are
reversed. These are distinct source objects, and neither overlaps the other
in the bounded source under this calculation. The current draft places both
anchors at `(127,243)`. Other pairs may represent intentional or harmless
overlaps; the count must not be interpreted as a confirmed defect count.

Three focused Python tests passed on macOS arm64, including seam wrapping,
pre-existing pair exclusion and noninteractive overlaps. The tool does not
test engine occupancy semantics, terrain, guards, movement, or map balance.

## Engine check of the two creature banks

The opt-in `VCMI_INSPECT_EARTH_BANK_OVERLAP` branch of
`MapFormat.InitializesExternalToroidalMapAndChecksPortalPairs` initializes the
actual draft with its HotA mods, reveals the surface for the test and stages
the same red hero near the selected bank. With
`VCMI_INSPECT_EARTH_BANK_VISIT`, two independent initializations use normal
pathfinder routes and server movement requests. The first selects
`creatureBank_10789`, follows `(127,242) → (128,243) → (127,243)`, and opens
an activity. The second selects `creatureBank_11365`, follows
`(125,243) → (126,243)`, and opens an activity. In each run the selected bank
was the top visitable object on its own entrance. Assertions check the top
object, accepted steps, final hero tile and presence of an activity.

Both runs passed on macOS arm64 using the test change on `de93533`. Their XML
reports are `runtime/earth-bank-first-final-2026-11-15.xml` and
`runtime/earth-bank-second-final-2026-11-15.xml` on TB5. An earlier separate
in-memory removal of the second bank left the first entrance reachable both
before and after removal; that alone does not prove equivalent behavior.

These observations show an approach and activity for each object in a staged,
fully revealed test state. They do not establish normal access from a starting
town, all approach directions, the activity's contents or completion, or
visual quality. No map repair was applied solely because of the mask overlap.

## Shared visitable cells on the current draft

The current `earth-accessible-seam-2026-11-16.vmap`, SHA-256
`67640eefaa91e9efcf04315bef66197d834e662652bd25bd10f234391c43e5a8`, was
checked separately from the earlier pair report. A read-only scan of its
serialized masks found 2,858 canonical cells containing at least one `A` or
`T` visitable cell, and none containing visitable cells from two distinct
objects. This counts both newly introduced and pre-existing overlaps, unlike
the original pair-difference report.

The opt-in `VCMI_INSPECT_EARTH_INTERACTIVE_OVERLAPS` inspection then loaded
and initialized that draft in the engine with the HotA profile. It scanned
every cell's actual `visitableObjects`, excluding heroes and boats, and
asserted that no cell contained two or more remaining objects. The test
passed on macOS arm64 with the test change on `b8955d1`; XML is
`runtime/earth-interactive-occupancy-final-2026-11-16.xml` under TB5. This
checks one initialized state and the serialized masks, not every possible
randomized state, hero/boat overlap, visibility ordering, or collisions of
an interactive cell with a nonvisitable object's blocking footprint.

## Artifact and tree overlap

On the same current draft, `artifact_11045` has a visitable mask cell at
`(71,258,0)` overlapping a blocking mask cell of `pineTrees_9100`. The
opt-in engine inspection initialized the map twice, fully revealed the map,
and placed the red hero at `(72,259,0)`. The actual pathfinder found the
artifact tile, and server movement toward it was accepted. The artifact was
the top visitable object and was removed from the map after the request, both
with the trees present and after the test removed the trees in memory. The
hero remained at the origin in both runs. Each final run asserted the object
was removed; it did not inspect inventory or prove access from a starting town.
The source map and saved demo were not modified.

An earlier exploratory run incorrectly required the hero to occupy the
artifact tile and an activity to open, assumptions inherited from the creature
bank check; it failed those assertions. The comparison above records the
observed object removal without treating that failure as a confirmed defect.
The two final runs passed on macOS arm64 using `be65072` plus the inspection
extension, with HotA enabled; XML files are
`runtime/earth-artifact-tree-{present,removed}-2026-11-16.xml` under TB5.
This is one overlap and does not establish that other candidates are safe.

A further repeat on `992b30a` plus an inventory assertion checked that the
hero did not already possess the artifact type and did possess it after the
request. Both tree-present and tree-removed variants passed, and each still
asserted removal of the map object. XML is
`runtime/earth-artifact-tree-inventory-{present,removed}-2026-11-16.xml` on
TB5. The previous limitations about staged placement and other overlaps apply.

## Two shipyards and an occupied town

Two additional candidate overlaps involve `shipyard_10611` with
`iceBlock_9660`, and `shipyard_11031` with both `hotaBushes_10247` and
`spruces_10255`. In separate initialized current-draft games, the test fully
revealed the map and staged the red hero near each shipyard. The normal
pathfinder and server movement reached its visitable cell with all decorations
still present. The server then built a boat at `(16,272,0)` and `(56,69,0)`,
respectively, and the test verified a boat object at each location. Both
passed on macOS arm64 using `b70e9e5` plus the diagnostic extension; XML:
`runtime/overlap-boat-final-shipyard_{10611,11031}-2026-11-16.xml` on TB5.
This does not prove either yard is reachable from a starting town, boat
boarding, every approach direction, or behavior after other map changes.

For `randomTown_7497`, whose visitable mask overlaps `flowers_10066`, the
pathfinder found a route from the staged neighboring tile. An exploratory
visit test expected the town to be the top object and failed: the initialized
tile was instead topped by `randomHero_11991`. The test did not remove that
hero or attempt a battle or exchange, so it does not establish that the
flowers block the town or that an unoccupied visit works. No changes were made
to the map for these checks.

## Seam monolith approach repaired

In the earlier draft, `monolithTwoWay_11148` at `(1,285,0)` had no adjacent
free, nonvisitable land tile in the initialized state. Neighboring cells were
occupied by `deadVegetation_9592`, `mountain_9590`, a monster or a resource;
the diagnostic therefore could not stage a hero beside it. This alone does not
prove it was unreachable after clearing an interactive object, by boat, or by
teleporting into it. In an in-memory experiment removing only the vegetation
opened `(0,285,0)`; from there normal server movement opened the teleport
dialog. Its adjacent `(0,284,0)` was free land and `(251,285,0)` was water;
actual travel to the approach from a starting town was not tested.

The reproducible preparation now removes that one nonvisitable decoration,
checking its expected anchor. Reassembling from the unchanged converted
source produced `earth-portal-approach-2026-11-16.vmap`, SHA-256
`f8add693e77bed3ee423481f9ecbb81ed9e8ddb75ecbc0d6161ca006c60e33b5`.
Against the previous demo, archive members other than `objects.json` were
byte-identical; decoded objects differed only by removal of
`deadVegetation_9592`. The monster, resource, mountains and monolith remain.

On the saved repaired map, a staged hero at `(0,285,0)` reached the monolith
and opened its teleport dialog; similarly staged checks opened dialogs at
`monolithTwoWay_6730` and `monolithTwoWay_11110` despite their decoration
overlaps. These three checks passed on macOS arm64 using `655a7d7` plus the
inspection and recipe changes; XML is
`runtime/portal-repaired-monolithTwoWay_{11148,6730,11110}-2026-11-16.xml`
on TB5. The earlier start-to-road-to-gate server route also passed on the
repaired map under its previously stated forced-victory and visibility
conditions: `runtime/earth-repaired-start-route-2026-11-16.{log,xml}`.
No graphical visit, choice of this monolith's destination, normal approach
from a start, or general overlap correctness is claimed.

The local demo profile now links to the repaired map; its prior map remains
on TB5. A separate seam preview generated from the repaired map has SHA-256
`581371aac879827ed86d14ff7283c48ce2b53d07e43f61baf60e6dd200638c2f` and
initialized with its portal pairs. Compared with the repaired demo its only
changed object is the staged red hero; the scenario text also changes. The
previous preview is retained next to the profile as
`earth-seam-preview-before-portal-repair.vmap`. In an isolated profile the
graphical client loaded the new preview and a human-controlled hero crossed
west from `(0,425,0)` to `(251,425,0)`, spending 75 movement points. The
observation is `runtime/earth-move-ack-qa-2026-11-16/observe-portal-fix.jsonl`
on TB5; the client was force-stopped. This is not a graphical visit to the
repaired monolith or the user's visual acceptance.

The `VCMI_INSPECT_EARTH_OPENED_MONOLITH_ROUTE` and
`VCMI_INSPECT_EARTH_OPENED_MONOLITH_EXECUTE` diagnostic subsequently started
at the red hero's original position on the repaired map. It followed the
previous route to `shipyard_5987` with two forced wins, built and boarded a
boat, and used the normal pathfinder to reach `(0,285,0)`. The 96-node
server-executed route included one further forced battle. The hero entered
`monolithTwoWay_11148`, selected the first exit in its teleport dialog and
arrived at `(0,292,0)`. The test passed on macOS arm64 with `a9a2146` plus
the diagnostic; log and XML are
`runtime/earth-opened-monolith-exit-2026-11-16.{log,xml}` on TB5. The test
revealed the map, modified creature aggression and fleeing, supplied extra
movement and forced three wins; it does not establish normal game balance,
unassisted travel, other exits or graphical portal behavior.

# Earth: start and terrain connectivity

On the integrated Earth road draft, SHA-256
`61f9f042612fb86aa611c64e57b2c2e6ee7a39409cf2b4eeb1fd74b8b00d40b7`,
the optional `VCMI_INSPECT_EARTH_START_CONNECTIVITY` inspection in `faa09b1`
initialized the real game, revealed the entire surface to the red player in
memory and ran the normal single-hero pathfinder from `randomHero_7305`.
It reported the seam road `(0,425,0)` and each of the four converted gate
positions `(2,388,0)`, `(249,388,0)`, `(2,416,0)` and `(249,416,0)` as
unreachable in that initial state. The pathfinder retains objects, guards,
existing boats and the hero's actual abilities; full fog revelation is a
test-only condition. A negative result does not rule out reaching the area
after building or boarding a boat, defeating guards, using another hero or
changing the state. The test also checked the two converted portal channels.
It passed as a diagnostic, not as a reachability assertion. Its XML is local
on TB5 at `runtime/earth-start-connectivity.xml`; the test build used `e8c11cf`
plus the inspection code.

The independent `tools/analyze_land_components.py` inspection ignores every
object, transition rule, ability, boat, battle and portal. It treats all
recognized non-water terrain as land and uses eight-neighbor wrapping on both
axes. Starting tile `(164,353)` lies in a 13,588-cell terrain component; the
road and all four gate positions lie in a different 7,100-cell component.
This demonstrates that land terrain alone does not connect these sampled
points under that simplified adjacency. It does not establish gameplay
isolation, because boats, teleporters and other mechanisms can connect them.
The tool rejects unrecognized terrain prefixes and non-toroidal maps rather
than silently guessing.

Earlier [portal checks](earth-portal-road-2026-11-15.md) exercised individual
approaches and exits with staged heroes and forced guard victories. Together,
these results identify a remaining demo question: whether a player can reach
the seam region through ordinary exploration and transport. They do not show
that the transformed Earth map is balanced or fully connected.

## Transport possibilities

The opt-in inspection extended in `1164236` also examined all initialized
shipyard and boat visitable positions. With the same fully revealed initial
state and selected red hero, no shipyard LAND node or boat SAIL node was
reachable. This is a pathfinder result for the initial game state, not proof
that a player cannot fight through obstacles, build a boat later, use a town
shipyard, or find another route. The test passed as a diagnostic on macOS
arm64 using the `877e872` build plus this extension; its XML is local to TB5
at `runtime/earth-start-transport.xml`.

A separate read-only terrain and serialized-mask inspection found seven
shipyard visitable cells on the start hero's 13,588-cell land component:
`shipyard_5987`, `shipyard_6236`, `shipyard_6414`, `shipyard_6682`,
`shipyard_7303`, `shipyard_7452` and `shipyard_7455`. A terrain-only flood
fill also found a shared 97,108-cell water component adjacent to both that
land component and the road's land component, using eight-neighbor toroidal
adjacency. These are geometric estimates: they ignore occupancy, approach
directions, guards, navigation rules, the availability and cost of boats,
and whether any particular landing is permitted. Thus they identify a
potential sea connection but do not establish a usable gameplay route.

## One shipyard and nearby guards

For `shipyard_5987`, the real engine reports visitable position `(180,376,0)`.
Several surrounding land tiles are free and allow approach, but none is
reachable by the initial pathfinder. The inspection in `142b525` finds a
33-step geometric route from the initial hero to a permitted adjacent tile,
using the engine's map occupancy and movement-direction checks but ignoring
guard battles. The first tile along that particular route that the actual
pathfinder cannot reach is `(177,358,0)`, guarded from `(178,357,0)` by
`randomMonsterLevel2_5933`. After the test removes that particular creature
through the server in memory, the pathfinder reports the shipyard reachable
from the original hero position. The source and saved map are not modified.

A second initialization followed the geometric route through ordinary server
movement requests with 30,000 test-only movement points, full surface vision,
and a forced battle victory. A different battle started earlier, leaving the
hero at `(170,356,0)` and the identified `randomMonsterLevel2_5933` still
present. The pathfinder still reported the shipyard unreachable afterwards.
This confirms that the first actual encounter on that route is not necessarily
the guard found at the first unreachable node. It does not establish that the
hero can win normally, identify all battles, visit the shipyard, buy a boat or
sail to the seam. Both diagnostic runs passed on macOS arm64 using `9daf069`
plus the inspection change. XML files are local on TB5 at
`runtime/earth-shipyard-removal-final.xml` and
`runtime/earth-shipyard-battle-final.xml`.

## Route to the shipyard and boat

On 2026-11-16 the opt-in `VCMI_INSPECT_EARTH_SHIPYARD_ROUTE` diagnostic
followed the same 33-step geometric route from the starting red hero using
server movement requests. The test revealed the surface, set creature
aggression and no-flee flags, gave the hero 30,000 movement points, and
forced two battle victories. It reached `(179,377,0)`, then visited
`shipyard_5987` at `(180,376,0)`.

With `VCMI_INSPECT_EARTH_SHIPYARD_BOAT`, the normal server `buildBoat` call
succeeded with the player's available resources at `(182,376,0)`, and the
hero followed a computed route to board it. After the test replenished the
hero's movement points and replanned with the surface fully revealed, the
pathfinder did not return a route to the seam road at `(0,425,0)`. Among
reachable nodes, the geometrically closest water tile was `(239,425,0)`
and the closest land tile `(1,431,0)`, six rows and one column from the road
using wrapped coordinates. This is a nearest-node measurement, not an
executed route to either cell or proof that the road is inaccessible after
further encounters or state changes.

The diagnostic passed on macOS arm64 using test source changes on `950e94e`;
the XML is `runtime/earth-yard-seam-closest-2026-11-16.xml` on TB5. The
server calls exercise movement, visits, boat construction and boarding, but
the forced wins, adjusted creature behavior, extra movement and revealed fog
mean this is not a normal playthrough or evidence of battle balance. The map
and its original source were not modified.

## Prepared access to the seam road

The closest reachable shore near `(1,431,0)` was hemmed in by active cells of
`mountain_7569` and `yuccaTrees_7568`, with `randomMonsterLevel3_6689`
guarding the route farther north. Removing only the guard in test memory did
not make the road reachable. Removing only the two decorations made the
guard reachable but not the road; removing all three made the road reachable.

The demo preparation test `MapFormat.PreparesEarthConnectedSeamRoad` now
removes those two specifically identified nonvisitable decorations, checking
their original anchors and preserving the guard. The full local recipe
produced `earth-accessible-seam-2026-11-16.vmap`, SHA-256
`67640eefaa91e9efcf04315bef66197d834e662652bd25bd10f234391c43e5a8`.
Compared to the previous integrated demo, decoded objects differ only by
removal of those two decorations; header, terrain and translation archive
members match byte for byte. The original source remains unchanged.

Using the saved map, the `VCMI_INSPECT_EARTH_ROAD_PREPARED` and
`VCMI_INSPECT_EARTH_ROAD_GUARD_ROUTE` diagnostic follows the earlier route
from the original hero start to the shipyard, constructs and boards a boat,
then follows the path to the guard. The server starts a battle at `(1,430,0)`
and the test checks that the defender is `randomMonsterLevel3_6689`. After a
forced victory the hero reaches `(0,425,0)`, crosses to `(251,425,0)` and
returns to `(0,425,0)`. The test passed on macOS arm64 using the test changes
on `9c82fa6`; XML is `runtime/earth-accessible-seam-crossing-2026-11-16.xml`
on TB5. It reveals all fog, changes creature aggression and no-flee flags,
forces three victories, and repeatedly replenishes movement; it does not prove
the battles are winnable normally, the length of a real game, or visuals.

The local demo profile's map symlink now points to this saved copy. The
previous `earth-connected-seam-road-verified.vmap` remains on TB5 as the
rollback target. No map or third-party assets are committed.

## Gate paths after reaching the road

The same saved demo was checked on 2026-11-16 with the HotA demo profile and
the `38cc375` test build plus `VCMI_INSPECT_EARTH_ROAD_GATE_CONNECTIVITY`.
After the diagnostic route above reached the road and crossed both ways,
the fully revealed single-hero pathfinder found no LAND route from `(0,425,0)`
to any of the four converted gates. This was a successful inspection, not an
assertion that a route should exist; XML is
`runtime/earth-road-gate-connectivity-2026-11-16.xml` under the TB5 build root.
The result applies to that one game state after the three forced victories,
with extra movement points and modified creature behavior. Other battles,
transport, portal traversal or game-state changes could enable a route; no
gate route was executed and wider regional connectivity remains unverified.

An additional diagnostic on the same map and state compared those results to
an eight-neighbor land flood fill using engine occupancy and permitted movement
directions but ignoring guard battles. It found a free geometric approach to
all four gates. For gates `5430`, `5431` and `10533`, the chosen approach was
also reachable by the real pathfinder; each gate tile was guarded. For gate
`10528`, the chosen approach was not reachable, and the first unreachable
cell on that particular geometric route was `(248,412,0)`, guarded from
`(249,412,0)`. The gate tile itself was guarded as well. This supports guard
encounters as an explanation to investigate, not a proof that every gate is
reachable after defeating guards: the diagnostic did not execute those routes
or win those additional battles. It passed using `577b66d` plus this inspection
extension; log and XML are `runtime/earth-road-gate-geometric-2026-11-16.{log,xml}`
on TB5.

## Executed route from the road through one gate

On the same current draft and HotA profile, an extended server diagnostic
continued the prepared route from the red starting hero: two forced victories
on the route to `shipyard_5987`, a built boat, a third forced victory near
the seam road, and a crossing of that road in both directions. From `(0,425)`
the normal pathfinder supplied a nine-step route to `(3,417)`, beside
`subterraneanGate_5431`. Normal server movement executed the route and began
a fourth battle on the final approach. After a forced victory and the
necessary dialog replies, a movement request entered the gate and a teleport
dialog selected `subterraneanGate_10533`; the hero arrived at `(249,416,0)`.

The test passed on macOS arm64 using `2e499a4` plus this diagnostic extension;
log and XML are `runtime/earth-road-gate-traverse-2026-11-16.{log,xml}` on
TB5. The test reveals the map, modifies creature aggression and flee behavior,
replenishes movement and forces all four victories. It verifies one server
route and one portal direction, not normal combat balance, a graphical
playthrough, or access to every region and gate. The map was not modified.

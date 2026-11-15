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

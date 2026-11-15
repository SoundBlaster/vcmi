# Candidate Earth map demonstration

This is an exploratory proposal, not an accepted design or evidence that the
map has been converted. The user proposed adapting [Earth Rules the World
2.0](https://h3maps.com/maps/2-0-8381/) by jabba1000. The listing describes
a 252×252 map with two levels and requires HotA 1.7.2 or newer. Its two level
previews depict parts of the world, including the Americas on the underground
level. The user's sketch places the Arctic between the original surface and
an inverted Americas section, with Antarctica meeting across the top and
bottom seam.

VCMI supports rectangular maps, and our branch has separately tested 36×23
and 23×36 toroidal maps; see [rectangular evidence](evidence/topology-rectangular-vmap-2026-11-15.md).
Stacking two complete 252×252 levels would yield a 252×504 single-level
starting canvas with the same number of tiles, but geography changes could
require other dimensions. Loading, performance and playability at that size
are not established by the smaller tests.

The editor can copy selected objects between levels, but its current copy
path does not copy a terrain region or provide a general region rotation.
A conversion tool could first assemble terrain, then transform object positions
without blindly rotating their sprites and interaction masks. It must handle
references, events, portals, terrain transitions, roads, rivers, seam-crossing
footprints, collisions and gameplay balance. Seam-adjacent tiles meet at the
same x coordinate on the top and bottom edges; crossings do not rotate or
mirror a hero. Art and connectivity along both seams need deliberate review.

The downloaded H3M is kept outside the repository on TB5 and has SHA-256
`767e28f90b79fceb08be5a93d3e3c9c4d4bf7ab9e6dac9fd95c91806de387ff5`.
Its header identifies HotA format version 7. In the core+vcmi+vcmi-test
environment, the original loader threw `vector` while reading the HotA
predefined-hero extension: the table was larger than the installed hero pool.
After handling default entries beyond that pool, loading proceeds further,
but fails during water-content calculation because terrain index 10 is not
available in that test environment. The test environment only enables a
minimal HotA-format fixture, not the HotA content mod. The user's Complete
installation on TB5 does contain original HotA resource files; their presence
alone does not establish that the VCMI HotA port is installed or active.
These observations do not establish whether a fully configured VCMI can
load or play this particular map.

## Load and VMAP round trip with the HotA port

The VCMI HotA port was cloned separately on TB5 from
[`vcmi-mods/horn-of-the-abyss`](https://github.com/vcmi-mods/horn-of-the-abyss)
at commit `6a5e21f3995996ae3d6083f8c94a1c8b0ff34cf3`. An explicit external
test profile enabled the port and its map-support dependencies without
changing the user's normal mod preset. In that environment,
`MapFormat.LoadsExternalLegacyMapForInspection` loaded the original H3M:
252×252, two levels, 12,249 object slots and 48 towns. The surface had 7,496
non-null objects, 1,846 road tiles and 170 river tiles; the second level had
4,753 objects, 1,330 road tiles and 144 river tiles.

An opt-in in-memory H3M → VMAP → VCMI loader round trip retained dimensions,
level count, town and named-instance counts. Every named instance retained
its type and anchor position; every tile retained its terrain, road and river
types and their used artwork variants and flip bits. Bit 6 of `extTileFlags`
changed on 6,952 tiles; bit 7 did not change. The source labels bit 6 as a
coastal flag, but the examined current engine uses computed coast checks and
does not directly read that bit. Its loss is recorded, not assumed harmless
for all consumers. Complete object properties, references, event behavior,
playability and visual correctness were not checked. The VMAP was not
published or added to the repository.

With the same HotA test profile, the existing
`MapFormat.ConvertsExternalLegacyMapWithWrappedFootprints` test also passed on
the original, still two-level Earth map. It enabled toroidal topology in
memory, confirmed at least one seam-crossing static footprint, saved and
reopened a VMAP, and compared named objects' IDs, anchors, dimensions and
per-cell covering, blocking and visitability masks. This is evidence for that
conversion path, not an adapted single-level map or a playable-session test.

The test accepts `VCMI_TEST_MODDED_ROOT` pointing to a directory containing
`config/modSettings.json` and `Mods/`; an invalid root is rejected before
initialization. `VCMI_INSPECT_LEGACY_H3M` selects the external map and
`VCMI_INSPECT_VMAP_ROUNDTRIP` requests the in-memory round trip. Optional
`VCMI_INSPECT_VMAP_OUTPUT_DIR` writes the archive as `map.vmap` only after
creating a new directory; an existing directory is rejected. The local copy
under `runtime/earth-map-research-2026-11-15/converted-source/` on TB5 has
SHA-256 `69e1f1aff0398a7042e7b158b1ac638ae3237b23d0a5a8927f0afcb7ef0c4023`.
It is a two-level conversion of the source, not the proposed demo; the bit-6
difference above applies.
Inspection also found that this raw VMAP's header has `mods: null`, although
its content references HotA entities. Before treating a derivative as a
distributable scenario, determine and declare its actual required mods and
verify missing-mod diagnostics. The editor has an object-mod assessment path,
but its source explicitly leaves terrain assessment as a TODO.
The ordinary VCMI mod and game settings retained their earlier SHA-256 values; ordinary
test runs updated the validation checksums in the separate test preset.

The loader change is covered by synthetic HotA v5 maps: default hero entries
past the installed pool load, custom entries for an unavailable hero report
its index, and an available customised hero is still created. The local full
suite passed 1,995 tests, skipped five optional tests and reported three
disabled tests. Its XML and log are under
`runtime/earth-map-research-2026-11-15/` on TB5. This suite uses the test
fixture mod; it does not demonstrate a full HotA installation.
The listing does not establish permission to redistribute the map or a
derivative; investigate the author's terms before publishing either.

## Candidate placement analysis

The read-only [`tools/analyze_stacked_map.py`](tools/analyze_stacked_map.py)
estimates object-mask overlaps after placing the rotated second level above
the surface on a torus. It interprets the serialized template masks and does
not simulate pathfinding, event order, terrain suitability or gameplay.

With the original anchor rotated, it finds 1,525 new pairs of blocking
objects compared with the clipped two-level source, including 349 occurrences
where a new pair overlaps a visitable cell. Keeping the first visitable cell
at its rotated position reduces these estimates to 1,274 new blocking pairs
and 245 such occurrences. Some objects have multiple visitable cells, so
preserving the first cannot preserve the whole interaction geometry. The
baseline contains 267 blocking footprint cells clipped by the original map
edges; wrapped cells can also contribute to differences. These counts are
candidate conflicts for engine/editor validation, not proven gameplay bugs.
Three small geometry tests cover rotation, clipping, wrapping and visitable
placement. No transformed map has been saved by this analysis.

## Terrain-only layout preview

[`tools/preview_stacked_terrain.py`](tools/preview_stacked_terrain.py) draws
the rotated underground above the unchanged surface using terrain minimap
colors. It writes a new SVG and refuses to overwrite a file. Its generated
preview is `runtime/earth-map-research-2026-11-15/stacked-terrain-draft.png`
on TB5. Inspection shows the Americas above Eurasia with Arctic regions near
the join, but also the underground's black rock areas and Antarctica as a
separate island near the top. The planned polar and seam edits have not been
performed. The picture omits objects, tile art, roads and rivers; it is not
an editor rendering, a playable map or evidence of seamless transitions.

The preview tool also classifies paired edge terrain without declaring a
water/land pair invalid. For this unedited layout, the 504 west/east pairs
include 157 pairs with rock on one side, 155 land/water and 192 water/water.
The 252 north/south pairs include 48 rock/water and 204 water/water. The
internal join between the two source levels has the same totals. These are
terrain-only counts; object occupancy, ships, coastline art and actual
crossing behavior need separate checks.

An optional second illustration selects the 4-connected snow component at
`(130,30)` in the stacked preview, removes those 2,304 source snow cells,
stretches its shape across the width and inserts new ocean bands around the
north/south seam. Its candidate canvas is 252×576, with 72 added rows; this
is a visualization choice, not a decided final map size. In the resulting
terrain-only boundary, 183 paired cells are land/land and 69 are water/water.
The preview remains blocked by the pre-existing rock areas elsewhere. It is
saved as `runtime/earth-map-research-2026-11-15/stacked-polar-draft.png` on
TB5. No object or gameplay data was transformed. The tool rejects a selected
snow component that touches the original preview boundary; eight Python
geometry and preview checks passed. Exact artwork, placing objects from the
original island, seam roads and playability remain unresolved.

The read-only [`tools/analyze_polar_objects.py`](tools/analyze_polar_objects.py)
compares the selected snow component with second-level source object masks.
For this map, 330 object anchors and 330 footprints intersect it; 117 objects
have a visitable mask cell on the island. They include 10 mines, 25 monsters,
28 random resources, two shipyards and a keymaster tent. This is a footprint
estimate on the original placement, not proof of reachability or a decision
about which objects should move. Ten combined Python geometry and preview
checks passed after adding the object analysis.

## Revised illustration: Africa and Greenland

The user pointed out that a piece of Africa appears on the opposite side of
the original map and asked for Greenland to be placed on the right. A further
terrain-only illustration rejoins the second-level east-edge land component
seeded at `(251,130)` to the surface in its original orientation. Of its 714
cells, 677 replace water and 37 overlap existing surface land, which is kept.
It then replaces 6,682 boundary-connected rock cells with ocean, moves the
Antarctic snow component into polar bands, and draws a new schematic snow
island at `(220,275)` for Greenland. The latter is an approximation made for
the illustration, not a verified transfer of original Greenland terrain.
The resulting 252×576 illustration is kept outside the repository as
`runtime/earth-map-research-2026-11-15/stacked-africa-polar-ocean-greenland.png`
on TB5. None of these operations edits or creates a playable scenario.

The fragment analysis finds 219 second-level object footprints intersecting
the source component, including 53 with visitable cells. These objects have
not been moved; overlaps and references need review when an actual map is
assembled. The newly drawn Greenland has no objects. Replacing rock in the
picture likewise does not establish that changing those terrain cells is safe
for objects or gameplay. Fourteen Python geometry, preview and analysis tests
passed; they check the tools, not in-game appearance or playability.

## Projection reference

The user identified [Jason Davies' Map Projection Transitions](https://www.jasondavies.com/maps/transition/)
as a visual reference: a rotated equirectangular view can bring a geographic
pole into the interior and arrange landmasses around it, resembling the
proposed layout. The page animates between projections and orientations, so
the user's screenshot is a reference to a particular appearance, not a
record of exact rotation parameters. An oblique equirectangular projection
is an established variant of the projection; see [NASA's projection list](https://www.giss.nasa.gov/tools/gprojector/help/projections/).

This does not make Earth's spherical topology toroidal: in a full-sphere
equirectangular representation, opposite ends of the projection's longitude
axis identify a meridian, while the two other edges represent distinct
projection poles. Joining those edges tile-for-tile is an intentional game
world alteration and needs separately designed terrain and traversability.
The candidate source map is hand-authored and split across two levels; it is
not established to have geographic coordinates suitable for automatic
reprojection. Transferring its objects, roads, events and references still
requires map-aware transformations and validation. The current sketch is not
calculated using the reference projection.

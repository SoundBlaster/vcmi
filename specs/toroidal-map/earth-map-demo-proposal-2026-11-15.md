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

The user also shared [Adams Hemispheres in a Square](https://www.map-projections.net/single-view/adams-hemispheres)
as a reference for a possible future spherical topology. That future mode is
outside the current toroidal feature: the relationship between hemisphere
edges, orientation after a crossing, polar behavior, distances, projection
distortions and game-grid representation would need their own design and
tests. The cited page identifies the depiction as a transverse aspect of the
Peirce Quincuncial projection. Referencing this illustration does not select
its geometry as an implementation or license its image for redistribution.

The user also identified the [Gall stereographic projection](https://proj.org/en/stable/operations/projections/gall.html),
as displayed in the projection-transition tool with a rotated globe, as a
reference for placing the Arctic near the center and Antarctica around the
edge. The pictured orientation is not captured by exact rotation parameters.
It is a candidate for the visual arrangement, not an automatic conversion of
the H3M. For the toroidal demo, manually align and check corresponding edges,
including ocean and any Antarctic land crossings; the projection alone does
not establish a continuous toroidal seam.

## Cross-level passages require explicit adaptation

The converted source has four `subterraneanGate` instances: surface anchors
`(3,100,0)` and `(3,128,0)`, and second-level anchors `(250,100,1)` and
`(250,128,1)`. There are also four separate `borderGate` instances, which
must not be confused with subterranean passages. The current
`CGSubterraneanGate::postInit` pairs gates only when their visitable positions
are on different levels. Thus merely relocating all four gates onto one
surface produces no cross-level partners; they cannot be treated as working
passages in the demo.

The source map's surface gates lie near the western boundary and the other
two in the eastern Africa fragment. One design candidate is to restore a
walkable physical connection around the toroidal seam and remove or replace
the redundant gate pairs. Another is to replace each pair with an explicitly
paired same-level passage. Neither option has been implemented or selected;
first inspect the terrain, footprints, access restrictions and actual source
gate pairing. Retaining object names alone would not preserve this behavior.

## Rotation and road rendering

The converted source contains 1,846 surface and 1,330 second-level road
tiles. `MapFormatJson` stores separate flip bits for terrain, roads and rivers,
so reversing the cell grid alone does not rotate their artwork. In addition,
`MapRendererRoad::renderTile` draws the upper half from the preceding row's
road and the lower half from the current row's road, whereas pathfinding
consults road data on the source and destination cells. Therefore blindly
rotating road flags on their original cells is not sufficient evidence of
either visual continuity or correct movement costs. Before exporting the
rotated Americas, design a road transformation or regenerate road artwork,
then test its rendered joins and movement costs independently, especially at
new seams. No road transformation is implemented by the terrain-color
preview.

## Diagnostic stacked VMAP

[`tools/assemble_stacked_draft.py`](tools/assemble_stacked_draft.py) builds a
first-stage single-level VMAP from two equally sized source levels, keeping
surface objects upright and rotating second-level coordinates and tile art.
It attempts to preserve each rotated object's first visitable cell and remaps
player main-town references to the corresponding new anchor. The input map is
not modified, the output must not exist, and unsupported nested coordinate
references are rejected. The tool's report explicitly lists unresolved work;
it is not a general-purpose converter or a playable-map claim.

On the Earth source, the local draft has SHA-256
`57689688e9a21a6f63ee86be40cde9a7a382b1571ad4d77edabf243895eadae2`
and is stored only on TB5 under
`runtime/earth-map-research-2026-11-15/draft-stacked/`. The source has 53
second-level object anchors outside its rectangle, each with at least one
blocking or visitable footprint cell inside; these anchors are canonicalized
in the draft. Its placement estimate reports 1,274 new blocking pairs, with
245 occurrences affecting a visitable cell. These counts are diagnostic
estimates, not individually confirmed gameplay conflicts. The draft still
has four unpaired subterranean gates and does not implement the proposed
African fragment, Antarctica or Greenland edits. It also inherits an empty
mod-dependency declaration that must be corrected before distribution.

With the isolated HotA test profile, the VCMI loader opened the draft as a
252×504, one-level toroidal map with 12,249 objects, 48 towns, 3,176 road
tiles and 314 river tiles; each declared main-town anchor resolved to a town.
An in-memory VMAP save/reload retained named object types and anchors and
terrain, road and river types and artwork flags. This checks format handling,
not map initialization, all object properties, visual road joins or gameplay.
Nineteen Python geometry and preview tests passed; the regular `MapFormat`
suite passed 14 cases with three opt-in cases skipped. The tool and inspection
changes were subsequently committed as `c805b47`; the test executable was
built from `2b53c74` with those inspection changes in the worktree. No adapted
map data has been added to the repository.

### Rejoining the eastern African fragment

The next diagnostic draft uses `--rejoin-east-fragment 251 130`. It moves
the 714-cell source land component in its original orientation onto the
surface portion before stacking: 677 cells replace water and 37 overlap
surface land, which is retained. All 219 second-level objects with blocking
or visitable footprint cells on the component are moved as whole objects,
including a random town, three shipyards, two subterranean gates and 53
objects with visitable cells on the component. The rule includes objects
whose original anchors lie outside the source rectangle. Instance names and
all object JSON fields except the position were unchanged in a direct
comparison of all 12,249 objects; the 219 chosen positions matched the
intended translation. References beyond the checked main-town anchors and
the gameplay effects of these positions have not been validated.

The result remains an unfinished diagnostic map at
`runtime/earth-map-research-2026-11-15/draft-stacked/earth-africa-diagnostic.vmap`
on TB5, SHA-256
`c0e71a1ce10b532c4317f809bae80ac64d78234758557436eab82bfe7ef0a6db`.
Compared with the original map's clipped footprints, the approximate
analyzer reports 1,341 new pairs of blocking objects and 249 occurrences
involving visitable cells. Compared with the earlier stacked draft, 130 new
pairs appeared and 63 disappeared; 128 of the new pairs combine a moved
fragment object and an original surface object. Twenty-five of those 128
pairs involve visitable cells. These are candidates for review, not proof
that every blocking overlap is a gameplay defect. The tool does not remove or
relocate conflicting objects automatically.

With the isolated HotA profile, VCMI loaded the new draft as one 252×504
toroidal level with all 12,249 objects and 48 towns and resolved the declared
main-town anchors. Its in-memory VMAP round trip preserved the tested object
anchors and types, terrain, road and river types and artwork flags. Twenty-one
Python geometry and preview tests passed. Neither this load nor the object
comparison starts a game or verifies event behavior, road appearance,
Antarctica, Greenland, gate pairing or the intended final map size. The
transfer tool change was committed as `0bab1e4`.

### Polar layout and schematic Greenland in the draft

The diagnostic assembler now optionally moves the selected 2,304-cell snow
component around new 36-row bands at both ends of the map, stretching terrain
to the 252-cell width. It moves 319 objects whose blocking or visitable mask
cells touch the source component, including all 117 with visitable cells on
it. The earlier count of 330 included objects whose *visible* mask alone
touches the component; those 11 are not automatically moved. Each relocated
object remains upright and at its original sprite size, with one selected
active cell mapped to snow. The tool checks all 319 selected cells against
the resulting terrain and reports seven mountains with active footprint cells
on water; those cells and surrounding approaches require review. It rejects a
polar component containing road or river overlays rather than multiplying
them during the stretch.

The diagnostic composition also changes 6,682 boundary-connected rock cells
to water. Six nonvisitable objects have active cells on changed terrain;
their appearance and passability are unverified. The tool rejects this
operation when a changed cell intersects the active footprint of an
interactive object, or when the rock carries a road or river. It adds the
391-cell schematic Greenland island at `(220,275)` only in clear water; the
new island has no objects. These edits preserve source instance names and
non-position object JSON fields in a direct comparison, but do not prove
that objects remain reachable or scenario references work during play.

The local composed file is
`runtime/earth-map-research-2026-11-15/draft-stacked/earth-composed-diagnostic-v2.vmap`
on TB5, SHA-256
`694c076791e953a8c36e3b28dfeda927139e2b8f483bda3811c32b89438d6302`.
A terrain-color rendering of the actual file is adjacent as
`earth-composed-diagnostic-v2.png`; it is not an in-game screenshot. The
loader opened it as a single 252×576 toroidal level with 12,249 objects and
48 towns; the tested VMAP round trip preserved named object types and
positions and terrain, road and river types and artwork flags. All 25 Python
geometry and preview tests passed. The generated file was reproduced in
memory using the committed `c5a65c4` assembler. The file is local and is not
part of the repository.

The paired terrain categories at the north/south seam are 183 land/land and
69 water/water; at the west/east seam they are 69 land/land, 36 land/water,
7 water/land and 464 water/water. These counts say nothing about matching
coast artwork or actual passability. The draft has 1,302 estimated new
blocking pairs and 250 visitable-cell overlap occurrences relative to the
clipped source; gates, mod declarations, roads, object interactions, visual
transitions and an actual playable session remain unresolved.

### Same-level passage prototype

The assembler can explicitly replace source gate pairs with two-way
monoliths while keeping each object's instance name, position and original
gate appearance and footprint. It verifies each requested pair against the
source map's gate-pairing order and rejects an already-used monolith subtype.
For this map, `subterraneanGate_5430` / `subterraneanGate_10528` use the
previously unused `monolith2` subtype, while `subterraneanGate_5431` /
`subterraneanGate_10533` use `monolith4`. This is a deliberate adaptation,
not full preservation of gate behavior: for example, a subterranean gate
can start an exchange with a friendly hero at the exit, while a two-way
monolith cannot.

Two mountain decorations obscuring portal visitable cells were relocated
to the nearest candidate within 16 cells whose active footprint is on dry
terrain, has no road or river and overlaps no other active footprint.
`mountain_3335` moved from `(0,389)` to `(6,387)` and `mountain_7529` from
`(2,417)` to `(245,426)`. No object was deleted. The estimate changed to
1,293 new blocking pairs and 248 visitable-cell overlap occurrences versus
the clipped source. Actual accessibility, road appearance and the effects of
the new mountain locations are not established by these geometric checks.

The local draft
`runtime/earth-map-research-2026-11-15/draft-stacked/earth-portals-clear-diagnostic.vmap`
has SHA-256
`75aff0d02d2eb994a591a13d183da8b979d1faea7e9c9f67093065e7c4895d65`.
The isolated HotA profile loaded and round-tripped it with 12,249 objects and
48 towns. The `MapFormat.InitializesExternalToroidalMapAndChecksPortalPairs`
opt-in test initialized an actual in-process game state, found exactly two
isolated two-entrance/two-exit channels, and used the server activity handler
to visit each portal in both directions. For those visits the test places a
hero directly at each entrance and chooses the matching exit; it does not
exercise ordinary approach or nearby guards. This test passed three times on
macOS arm64, and 28 Python geometry and
conversion tests passed. `MapFormat` normally passed 14 cases with four
optional inspections skipped. The assembler change is `46cd49c` and the
opt-in game-state inspection is `4d22789`; the tested executable was built
from `0f0d2e0` with the inspection changes in the worktree.

The opt-in inspection also supports `VCMI_INSPECT_APPROACH_PORTALS`. For the
same candidate, it placed a hero on a free adjacent cell, made a normal server
movement to each of the four entrances, and selected the corresponding exit.
All four approaches started guard battles; the test set the guards not to
flee or join, forced victory using the server's battle test hook, answered
level-up prompts, and checked arrival at the paired exit. With
`VCMI_INSPECT_REQUIRE_ALL_GUARDS`, the four battles and traversals passed in
three consecutive runs. This is stronger than direct visits, but still does
not prove an approach from a starting town, normal battle play, graphics,
remote networking, or the rest of the scenario. The test change is
`0cbde2c`; it was built from `505ef7f` with that change in the worktree.

### Relocating mountains that cover interactive cells

The `--repair-mountain-interactions` option addresses one subset of the
estimated new overlaps. It selects plain noninteractive mountain objects
involved in newly overlapping interactive footprint pairs, excluding any
mountain that already participated in an interactive overlap in the clipped
source. It tries a deterministic nearby position within 16 cells, requiring
the entire active footprint to be on dry terrain without roads, rivers or
other active footprints. A selected polar reference must remain on snow.
Mountains for which no such position exists are reported and left untouched.
The procedure is opt-in and its edits may alter passages or gameplay; it
does not decide whether all reported overlaps are defects.

On the candidate with the two manual portal-area relocations, 36 mountains
qualified; 33 were moved and `mountain_4341`, `mountain_7900` and
`mountain_8145` had no available position under those conditions. The
number of estimated new interactive-cell overlap occurrences fell from 248
to 211, and new blocking pairs from 1,293 to 1,213, compared with the
original clipped map. A direct comparison of the two drafts found exactly 33
changed objects, all mountains with unchanged non-position JSON fields, no
new blocking object pairs, and no moved mountain with a pre-existing source
interactive overlap. Pair-based analysis can still miss additional overlap
cells between a pair that already interacted in the source.

The local result is
`runtime/earth-map-research-2026-11-15/draft-stacked/earth-mountain-repair-diagnostic.vmap`,
SHA-256 `277c29ac2fa7f3594d8bf47bbb19c13eefba9fa15a8a9f9b4197028499ffaa76`.
VCMI loaded and round-tripped it with the same 12,249 object and 48 town
counts. The in-process portal-approach test still passed three times with
four forced guard victories per run. The current assembler reproduced the
tested header, terrain and object data, and all 31 Python tests passed. The
repair tool is `f80801e`. These results do not verify ordinary routes from
starting towns, map balance, visual layout, unhandled interactive overlaps,
or the rest of the game.

The external-map initialization test now optionally inspects one initially
placed, non-garrisoned land hero per player using the server-side pathfinder.
It selects a reachable adjacent empty, unguarded land tile and sends the actual
standard move through `CGameHandler`, without artificially repositioning the
hero or adding movement points. With `VCMI_INSPECT_START_ROUTES=1` and
`VCMI_INSPECT_REQUIRE_START_MOVES=8`, all eight players' heroes found such a
tile and moved there in each of three in-process runs on the map identified
above. The observed number of reachable neighboring tiles was between two
and seven. This checks one short move per player, not longer routes, fog
behavior, player-facing controls, balance, or overall scenario playability.

### Diagnostic mod requirements

The initialized draft's object handlers came from `core` and twelve HotA
scopes; terrain came from `core`, `hota.highlandsterrain` (7,291 tiles), and
`hota.wastelandterrain` (986). A separate scan of serialized header and
objects found references to `hota.heroes` and `hota.interference` as well.
Together with the root `hota` scope, these form a candidate list of 15
required mods. This is a conservative inventory of observed identifiers and
handlers, not proof that every asset or indirect dependency has been found,
nor that each listed mod is strictly necessary for every game configuration.

The assembler accepts repeated `--require-mod MOD_ID` arguments and adds
unversioned identifiers in the legacy-compatible `mods.allOf` form. It
preserves existing requirements and rejects conversion of modern versioned
metadata instead of silently discarding versions. The local
`draft-stacked/earth-required-mods-diagnostic.vmap` has SHA-256
`480972015e1de5df54547c743451b0b19ffa00f95eb550efa6fcb54932ce87f4`;
its archive members match the preceding mountain-repair draft byte for byte,
except for the header's `mods` field. It remains local pending source-map
licensing and playability review.

In the isolated HotA profile, VCMI's header compatibility check reported
15 declared mods and none missing; full initialization and eight initial
server moves also passed. With the regular test preset, which has no HotA,
the same header check reported all 15 missing. A VMAP save/reload retained
the mod identifiers and their unversioned status, as well as the checked
object positions, terrain and artwork. The 33 focused Python tests passed.
These checks exercise the engine API, not the graphical launcher's display.
Because the requirements are unversioned, they do not enforce compatibility
with a particular HotA release.

### Restoring mine guards after rotation

Twenty newly overlapping mine/monster pairs shared the same source pattern:
the monster's visitable cell was immediately south of its mine's entrance on
the original underground level. Rotating the entrance coordinates while
keeping the mine sprite upright put each monster on a blocking cell of that
mine. The optional `--repair-mine-guards` restores this specific relative
placement only when the source relationship is unambiguous, the monster had
no original interactive overlap, and its destination is a free land cell
without a road or river. It skips other cases rather than guessing. This can
still change which surrounding cells the monster guards and requires review.

For this draft it moved 20 guards and skipped none. Comparison with the
preceding draft found exactly 20 changed objects, only their `x` and `y`
fields changed, unchanged header and terrain, no new blocking or interactive
object pairs, and 22 removed pairs of each kind. The estimate of new
interactive overlap occurrences relative to the source fell from 211 to 189;
this metric remains an estimate, not a count of confirmed gameplay bugs.

The result is `draft-stacked/earth-mine-guards-diagnostic.vmap` on TB5,
SHA-256 `d974bd9f960f1cd2b3a6ce293139e58e1b0270cd231af7f8b41a4abc3641ed82`.
After real game-state initialization, VCMI recognized all 20 restored
monsters as guarding their respective mine entrances and none occupied a
blocking tile of its mine. All eight starting heroes again made one server
move. The VMAP round trip passed and the four portal approaches still passed
three runs with forced guard victories. The 35 focused Python tests passed.
These tests do not establish complete routing or balance and do not play out
the twenty mine battles.

The same source-relative pattern also appeared at 24 other buildings and
towns. The separate `--repair-other-guards` option applies the same checks to
an explicit set of nine object types. It moved 19 guards and skipped five:
four proposed destinations contain blocking decorations, and one contains a
road. The latter guard also stood on a road in the source, but the tool does
not yet verify preservation of road semantics and therefore leaves it alone.

The resulting local `draft-stacked/earth-object-guards-diagnostic.vmap` has
SHA-256 `beec069f1c872e2cfeb86de5e20d4117dd173f98f27885a7d332701ea54ae24f`.
Only those 19 monsters' coordinates changed from the previous draft; header
and terrain did not change. The comparison found no added blocking or
interactive object pairs and 20 removed pairs of each kind. The estimate of
new interaction occurrences is 169, down from 189. On three initialized
game states, the engine recognized all 39 moved monsters as guarding their
respective objects; all eight starting heroes made one move, and all four
portal approaches passed with forced guard victories. Mod requirements and
the VMAP round trip also passed. The five skipped guards and remaining object
overlaps still need investigation; these tests do not establish complete
playability or demonstrate the ordinary battles.

The road case was subsequently checked against the original underground
terrain: the guard stood on a `pc` road, as does the intended destination.
The guard repair now permits a road overlay only when the source and target
have the same terrain and road types; it still rejects rivers and different
overlays. No terrain or road tile is changed. This moves `monster_11082` to
its source-relative position at the entrance to `randomTown_11080`, leaving
four blocked cases. Compared with the preceding draft, exactly that monster's
coordinates changed; one blocking and interactive pair disappeared and no
new pairs were added. The estimated new interactive occurrences are 168.

The local output `draft-stacked/earth-road-guard-diagnostic.vmap` has SHA-256
`313596f80b2bab9364b2710e74bd362023c7777bcb4f10347c883eb19b231419`.
Thirty-seven focused Python tests passed. In three game-state runs the engine
recognized all 40 restored guards, eight initial moves succeeded, and all
four portal approaches succeeded with forced guard victories. The map also
passed the checked VMAP round trip. Matching road type does not prove visual
road continuity or the full pathfinding behavior of the scenario.

For the four blocked targets, the diagnostic assembler now supports
explicitly relocating selected plain mountains and spruces that had no source
interactive overlap. Placement avoids other active footprints, water, rock,
roads, rivers and explicitly reserved destination cells. With the four guard
cells reserved, both mountains and both spruces found placements and all four
remaining guards returned to their source-relative entrance positions. One
mountain moved 13 tiles horizontally and 14 vertically; the effect on routes
and landscape requires human review, so this is still a diagnostic proposal.

The resulting `draft-stacked/earth-reserved-guard-diagnostic.vmap` has SHA-256
`58e7b7e6bf94c117d350028757c9cf1c7160bf3107fe435cf003306d3dbd74a6`.
Compared with the preceding draft, exactly four decorations and four guards
changed position; their other fields, the header and terrain were unchanged.
No blocking or interactive pairs were added; eight blocking and four
interactive pairs were removed. Estimated newly overlapping interactive
occurrences fell from 168 to 164. In three initialized game states, all 44
restored guards protected the intended objects, all eight initial hero moves
and four portal approaches passed, with forced portal-guard victories. The
checked VMAP round trip and 39 focused Python tests also passed. Other
overlaps, geographical accuracy, actual battles and long routes remain
unverified.

The overlap estimate must not be read as a count of inaccessible objects.
For three distinct resources whose visitable cell overlaps blocking flower
decorations, an opt-in server test placed a hero on a free adjacent cell,
gave it movement points, visited the resource and checked the resource amount
and object removal. In these cases the hero stayed on the adjacent cell and
then moved to a second free cell. This uses an artificial starting position,
does not prove a route from a starting town, and does not check whether the
resource is visually identifiable.

An additional resource, `randomResource_11936` at `(29,114)`, could also be
collected from an artificially selected adjacent cell. The test did not find
an adjacent free, unguarded approach with a second free, unguarded cell to
depart to; a mine guard is nearby. This does not prove the resource or hero
is trapped: an ordinary route or battle may change the result. The case
remains open rather than being automatically edited.

The `earth-reserved-guard-diagnostic.vmap` terrain data contains no road
tiles on any of its four outer edges. It therefore cannot demonstrate a road
crossing a seam, even though it contains roads elsewhere. The separate
[Toroid Crossroads visual record](evidence/topology-preview-f0f7f4a-2026-11-15.md)
covers a different test fixture; it does not validate a seam road on this
Earth draft or replace the user's planned continuity review.

An in-process server test selected unobstructed land tiles on this draft and
made ordinary hero steps in both directions across each axis: `(0,425)` ↔
`(251,425)` and `(0,0)` ↔ `(0,575)`. It passed three runs after placing the
hero artificially at each edge and granting movement points. It does not
show a route from a starting town, diagonal corner movement, GUI response or
visual continuity.

At the four literal corners, the two `x=0` cells are snow and the two
`x=251` cells are water, so an ordinary on-foot diagonal corner crossing is
not available on this particular draft. The visible footprint of
`warehouses_11184` also touches the two southern corner cells. The terrain
and object artwork were left unchanged pending a deliberate map edit and
visual review; the synthetic toroidal corner tests remain separate evidence
for the engine.

The same initialized Earth draft also passed bounded sailing tests in both
directions across each axis. The test artificially placed a hero on clear
shore, created a boat in adjacent clear water and gave the hero movement
points; the server then handled boarding, sailing across the seam and back,
and disembarking. The checked water pairs were `(0,1)` ↔ `(251,1)` and
`(152,0)` ↔ `(152,575)`.

For the literal corners the test boarded across the horizontal seam and then
disembarked diagonally across both axes: `(0,575)` → `(251,575)` → `(0,0)`,
and `(0,0)` → `(251,0)` → `(0,575)`. Each sailing and corner scenario passed
three in-process runs. These are actual server commands on the loaded draft,
with artificial hero and boat setup; they do not establish an ordinary route
to those positions, a complete boat journey, GUI behavior or visual seams.
The normal `MapFormat.*` suite passed 14 tests and skipped five optional
external-fixture tests without their environment variables.

For each land seam crossing above, a further test confirmed that the
destination began hidden to the moving player's team and became visible after
the step. It serialized the initialized game state and header into a local
save, loaded the game-state chunk into a fresh `CGameState`, and compared the
hero position, toroidal flag, dimensions, object and town counts, and that
team's fog on all 145,152 map cells. The restored server handler then moved
the hero back across the same seam. Both axes passed three in-process runs;
temporary saves were removed. This does not test a full graphical save/load,
server activity state, other teams' fog or a networked session.

The sailing fixture also saved after crossing each seam while the hero was
aboard. A fresh game-state load retained the hero's position, boat identity,
map topology and all 145,152 fog cells for that player's team. A new
in-process server handler then sailed the hero back across the seam and
disembarked on the original shore. Both axes passed three runs, and the land
save test passed again after sharing the save/load helper. As above, the hero
and boat were set up artificially and the full server/UI session was not
saved or reloaded.

The fog comparison was then expanded to all eight valid players: on each
land and sailing save it checked team identity and 1,161,216 player-cell
values after loading. Both axes passed three runs for each movement mode,
and the checks passed once more with explicit team-identity validation.
Players sharing a team contribute repeated views to that total; this is a
game-state comparison, not proof of fog delivery to network clients.

The seam fixtures were also run with two independently loaded game-state
replicas. Server client-packets were encoded through `GameConnection` and
decoded separately against each replica before application; no socket was
used. For land movement, both replicas matched the server's hero position,
movement points and all eight players' team fog views after crossing and
returning on both axes. For sailing, they also matched the boarding state,
boat identity, disembarkation and the location of the boat left at sea.
The fixture set the same movement-point allowance on the replicas when it
reset the server hero's allowance after boarding. Each land and boat scenario
passed three runs; an additional run verified the left-behind boat position.
This checks packet serialization and application with an in-process capture,
not TCP transport, a real lobby or remote graphical clients.

The initial game state was also tested without relocating heroes, granting
movement points or artificially revealing fog. For one starting hero of each
of the eight players, the pathfinder selected a visible, unguarded resource
reachable during the current turn through visible, unguarded land cells. The
server executed the complete two-to-five-step planned route and collected
the resource. For each route two independently loaded replicas decoded the
server's packets and matched the hero position and movement points, resource
removal and credit, and all players' fog views. All eight routes passed in
each of three in-process runs. The player actions were executed sequentially
in the same game state, and the replicas used a memory transport rather than
TCP. This does not prove the rest of the map is connected, balanced or
playable, and it does not establish what a graphical player sees.

In eight separate fresh initializations of the same draft, Nullkiller2's
hero manager and pathfinder were given the corresponding visible resource
as a target, with hero chains and Dimension Door disabled. Each player had
a current-turn plan. The test executed each selected `ExecuteHeroChain`
through an in-process `CGameHandler`, serializing AI requests and forwarding
movement, reveal, object-removal and acknowledgement notifications to the
gateway. All eight runs collected the chosen resources without a rejected
movement request; hero positions, movement points and fog were not altered
before planning. The test chose the target and invoked the plan: it does not
establish autonomous goal selection, a complete AI turn, combat, TCP or GUI
behavior on this map.

A subsequent diagnostic attempted a complete autonomous Nullkiller turn on
the same draft. This was an exploratory, uncommitted extension of the test
client based on `3b366d1`, rather than a passing acceptance test. Nullkiller
visited `creatureGeneratorCommon_6252` (a wineyard) and received a blocking
dialog. Once the experimental client forwarded the dialog and registered its
answer request, the server acknowledged that answer successfully. The server
had zero active battles immediately after that acknowledgement, but the
test did not complete within 12 seconds. A successful acknowledgement alone
does not show that the associated activity has finished: replies can be
accepted for later resolution. The fake client's event and activity handling
is incomplete, so this observation does not establish a game defect or a
completed AI turn. Full autonomous turns remain unverified.

The diagnostic patch and log are kept on TB5 under the Earth-map research
runtime directory as `ai-full-turn-experiment.patch` and
`ai-dialog-battle-run.log`; the patch SHA-256 is
`5a1140d01a21d18cdda93a6631acfe09be7c6d677070d8cbcb299c04cdf152a0`.
This time-limited experiment is not enabled in the committed test suite.

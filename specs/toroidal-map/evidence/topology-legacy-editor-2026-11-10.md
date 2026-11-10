# Legacy map conversion in the editor

On 2026-11-10, on macOS arm64, the locally built ordinary editor was operated
using computer use. The application displayed build label `1.8.0.4cc2e9c`;
the editor executable SHA-256 was
`4a977d8f24694cc7bd8ec4e216c6ba0ff03e1ee41ebd0b44939cabfa594af635`.
Dynamic libraries were loaded locally from the TB5 build environment, so the
label alone is not a complete binary identity.

A synthetic two-level SoD H3M, with one hero, one town and a water tile,
opened with **Connect opposite map edges** unchecked. After checking it, the
editor saved a separate `.vmap`; reopening that file through the editor showed
the checkbox still checked. Its archive header contains `versionMajor: 4`
and `topology: toroidal`. The original H3M was not overwritten.

- Input `LegacyH3MCanBeOptedIntoToroidalVmap.h3m`: SHA-256
  `480ce53b6584c581a86d56ad0bf127b6e7991014612b9c6f0f57548377103efe`.
- Saved `toroidal-legacy-fixture-check.vmap`: SHA-256
  `ea395dd2be18cf4ceb26b6d918865e6a46ed130c60bb67bbe7e499aaccdd5baf`.

The automated `MapFormat.LegacyH3MCanBeOptedIntoToroidalVmap` test also
loads this synthetic legacy format as bounded, switches topology, serializes
and reloads it, and checks map dimensions, both levels, hero and town counts,
and the water tile. Together with the versioned JSON test it passed two tests
(`legacy-toroid-conversion-final-tests.log` on TB5).

For a separate `Dead and Buried` demo H3M (SHA-256
`3536cc09a8c3a7dc4602913ba58e63e2aa0f72da21df76ae55e6bed2edbd794e`),
the editor showed the opt-in checkbox initially unchecked. After enabling it,
the save attempt warned of critical map problems and then rejected the save:
`Toroidal map object crosses an unsupported seam: subterraneanGate_3`.
This verifies one instance of the provisional static-object restriction, not
that all such objects are detected or that all other legacy maps can convert.

The synthetic map was not played in this check. The editor did not repair
terrain, roads or object placement, and neither this test nor the successful
save proves that an arbitrary converted map is balanced or visually seamless.
Game resources and test artifacts remain local on TB5.

## Boundary objects in the demo map

A later local diagnostic loaded the same H3M using the normal H3M loader,
attached the editor callback, set the topology flag in memory and enumerated
objects for which `CMap::hasUnsupportedToroidalFootprint` returned true. For
each object it counted the visible, blocking and visitable mask cells outside
the original rectangle. The diagnostic was temporary instrumentation, not a
permanent automated test; its output is `legacy-seam-diagnostic.log` on TB5.

It found 98 objects with outside visible cells, of which 66 also have outside
blocking cells and none have outside visitable mask cells. The first reported
gate has three outside visible cells but no outside blocking or visitable
cells. Other affected objects include mountains, trees, lakes and terrain
decorations, including objects whose anchor itself is outside the rectangle.
These counts exclude heroes and boats because the existing restriction does.

The editor's save error reports the first rejected object, not necessarily the
only one. Wrapping previously clipped object masks can introduce graphics or
obstacles at the opposite edge; leaving them clipped yields different seam
behavior. This observation does not decide which transformation an author
wants, prove the resulting layout is playable, or establish which subset of
these objects can safely be enabled. The restriction is still in force.

## Fully clipped occupancy during topology conversion

On 2026-11-10 a synthetic map test placed an object with mask `A0` at
`(0, 5, 0)`: its only blocking and visitable cell is at `(-1, 5, 0)` and is
clipped in bounded mode. A second object with the same mask was deliberately
left hidden. After switching topology, the shown object's expected canonical
tile `(9, 5, 0)` had neither its blocking nor its visitable entry; the hidden
object remained absent. The existing shown/hidden rebuild test passed in the
same run. The new regression is temporarily disabled until the map can retain
the distinction between an explicitly hidden object and an object whose entire
occupancy was clipped; it was run enabled to establish the failure.

The current rebuild discovers shown objects from the existing tile occupancy
index, which cannot distinguish these two cases when no indexed cells remain.
This is one reason the static-footprint save/load restriction must not yet be
lifted. The test is synthetic and does not establish how many objects in the
demo H3M are affected. It does not test rendering or map serialization.

The editor subsequently gained an explicit authoring rebuild mode that takes
all non-null entries in the map's object list as placed objects. Its synthetic
test checks that the clipped blocking and visitable cells appear after enabling
the torus and disappear after returning to bounded topology, for two cycles.
The default rebuild mode still uses the pre-existing occupancy index and its
test verifies that an explicitly hidden object stays hidden. The broader
shown-versus-hidden case remains a disabled regression for the default mode.

The editor and test targets built on macOS arm64, and the focused run of map
draw-order and legacy-conversion tests passed 14 tests with the one regression
disabled. This checks the map model and the editor build, not interaction with
the editor UI. The static-footprint load/save restriction remains in force.
The full local suite passed 1,427 tests, skipped two optional prototype-input
tests and reported five disabled tests (`full-suite-authoring-rebuild.log` on
TB5). SpecPM validation reported no errors or warnings.

## Two real H3M round trips and demo launch

The later `MapFormat.ConvertsExternalLegacyMapWithWrappedFootprints` test
loads a caller-supplied H3M, explicitly enables the torus with the editor's
all-objects rebuild, saves VMAP and reopens it. It checks that at least one
static footprint crosses a seam, compares instance names, object types,
anchors, dimensions and visible/blocking/visitable mask cells, and compares
the sets of blocking and visitable object names for every tile on every level.
It passes with both of these separate two-level maps:

- Demo `h3demo.h3m`, SHA-256
  `3536cc09a8c3a7dc4602913ba58e63e2aa0f72da21df76ae55e6bed2edbd794e`:
  this is the file rejected in the earlier editor check above.
- Complete `Dead and Buried.h3m`, SHA-256
  `cc23096e678a2e3bd9cf62445d7bba1828924222969b6ca5828d4805886fc162`:
  a different file with the same map name.

The generated demo VMAP's SHA-256 is
`65fc7c17a561273af9ae1b2b833d7b37a5ed7af37c195120f3702b9952e94b5d`.
The same diagnostic client and library identified in the
[wrapped-footprint record](topology-wrapped-footprint-vmap-2026-11-10.md)
started that VMAP using temporary local copies of the Complete archives.
Computer Use showed the scenario start message and the game map. After
revealing the map, westward then northward camera scrolling produced views
whose minimap viewport spanned both map edges. These were discrete visual
observations of the surface only, with no hero movement, gameplay completion,
or per-object render comparison. The session was closed without saving and
temporary resource copies were removed; the profile links were restored.

The test also compares the ordered visitable-object lists before and after the
round trip; it does not assert the presence of a particular interactive
overlap. It does not establish that all overlapping objects remain accessible
or that an arbitrary legacy map is balanced, traversable or artistically
seamless when converted.
Neither source map nor game resources are included in the repository. The
test skips when no external H3M path is supplied.

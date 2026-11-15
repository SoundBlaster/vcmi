# Editor flood selection across seams

The editor's Fill selection previously stopped at the physical map rectangle,
even when the map was toroidal. It now enumerates canonical four-connected
neighbors through the map topology, while preserving the existing terrain,
road, river and selected/unselected barriers. A bounded map still stops at
its physical edge. Each accepted tile is queued once; levels are not joined.

`MapTopology.EditorFloodSelectionRespectsTopologyAndBarriers` checks wrapped
corner regions, bounded controls, add/erase selection barriers, a whole-map
fill, level identity and invalid starts. The focused topology and VMAP seam
tests passed; the macOS arm64 editor and test binary built from `dd4b1a0`
plus the change. Build warnings included duplicate libraries and missing
Conan search paths. The complete suite passed 1,726 tests with three disabled,
using the prototype save, VMAP and legacy H3M inputs. Reports are
`runtime/editor-fill-seams-2026-11-15.xml` and
`runtime/full-suite-editor-fill-2026-11-15.{log,xml}` on the TB5 build root.

In a signed test editor, a separate copy of the 36×36 `Toroid Road Axes`
fixture was opened. A left-click Fill on the dirt patch at the west edge
selected the matching patch at the east edge. Applying snow changed both;
after save and restart, the editor displayed the changed terrain and both
halves on its minimap. The same tool selected a horizontal dirt road from
one edge through the other. Applying gravel changed both visible segments;
the saved VMAP's row 3 contained gravel at columns 0–3 and 33–35. The
original fixture retained SHA-256
`10039a23a4376106db02128141b6ae8c77b8b84065ce6117c278c99284b5e56b`;
the modified test copy is `runtime/editor-fill-test-2026-11-15/road-axes.vmap`
and has SHA-256
`ecd38b522607a3739faccab0a01b5a8773bd05323aeaeec98def4bc67a6d91f1`.
This copied prototype logs missing test-only spell identifiers and two empty
animation failures; it is not a clean playable demo.

The live check covers one surface horizontal seam and one road type change;
it does not establish GUI fill on the vertical seam, corner, underground,
other terrain rules or a visual gameplay test of the saved copy. The editor
screenshots were observed during the session and were not retained.

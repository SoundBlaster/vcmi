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

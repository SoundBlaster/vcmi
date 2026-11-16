# Jebus Cross: initial seam connectivity probe

On macOS arm64, `RandomTemplateProbe.GenerateJebusCross` generated and
reloaded a bounded 108×108, single-level map with four players, no water,
template `vcmi:Jebus Cross` and seed 12345. The template was the bundled
`Mods/vcmi/Content/config/rmg/hdmod/jebusCross.json`, SHA-256
`f633c6a4e6d3f4ddce6509f50a1adb80acb24a31a498dc7afba7ffc160b556c6`.
The template defines four player starts, a treasure zone and guarded links
from each start to that zone. This is Jebus Cross, not Jebus Outcast.

The local VMAP is
`runtime/editor-toroidal-scroll-qa-2026-11-16/Maps/jebus-cross-seed-12345.vmap`
under the TB5 VCMI build root; its SHA-256 is
`8ffae67b41fdb7b3081528ff6d062639a9abb94f0c2163d04265befb55849d5b`.
The checked-in [grid](data/jebus-cross-seed-12345.walk) has SHA-256
`3fa1b9ce581632a771be84268a88e12c06750b0d0521867717a99b7114a24dfd`.

For each tile, the probe records land that the engine reports as enterable,
without blocking or visitable objects and without a monster guard according
to `CMap::guardingCreaturePosition`, with wrapped guard lookup enabled. It
does not mutate the saved map. The comparison tool labels eight-neighbor
components without edge wrapping and then reports pairs joined by an added
wrapped edge. On this grid, 4,254 cells form 47 bounded components; eight
distinct pairs acquire at least one seam connection. One cardinal example is
`(3, 0)` to `(3, 107)`, connecting components of 951 and 851 eligible cells.
Diagonal seam candidates are included, including corners.

These are **candidate shortcuts**, not proven hero routes or proven bypasses
of a particular template connection. In particular, the model treats all
visitable tiles as barriers; it does not model hero movement costs, direction
rules, fog, boats, spells, teleports, dynamic objects or all corner cases.
Wrapped footprints of previously clipped objects were not rebuilt in this
probe. The VMAP does not contain the generator's per-cell zone IDs, so these
components have not been identified as specific template zones or starts.
The 951 and 851 cells are component sizes, not route lengths. A server or
pathfinder scenario and a generation-time zone export are needed to establish
whether a candidate actually bypasses intended guards.

The same test ran again with seed 12345 and produced a different grid
(SHA-256 `4fe60a6c62bf54b58f480e60379433b350bf11e88ee1770e6182d60b7f34764d`),
48 components and six candidate component pairs. The local generator config
uses `singleThread: false`, and the generator executes modifiers in a TBB
task group; this observation does not isolate the exact source of variation.
The seed alone was therefore not sufficient to reproduce the first grid in
this configuration. Preserve the grid and hashes; do not imply that simply
rerunning the seed reproduces it.

The generation/reload diagnostic passed, as did the four synthetic comparison
tests (edge, diagonal corner, already-connected control and malformed input).
To reproduce the analysis of the preserved grid:

```sh
python3 specs/toroidal-map/tools/compare_walk_seams.py \
  specs/toroidal-map/evidence/data/jebus-cross-seed-12345.walk
```

To generate a new local sample, set `VCMI_RMG_PROBE_OUTPUT` to an output
VMAP path and run `vcmitest --gtest_filter=RandomTemplateProbe.GenerateJebusCross`
with initialized VCMI game data. The test writes a `.walk` sidecar; it is
opt-in and does not run during ordinary test invocations.

# Wrapped hole lookup: review and measurement

The review observed that a toroidal `isHoleAt` miss scanned every raw hole
position after a direct canonical lookup. That is correct for legacy objects
whose raw anchors lie outside the map, but scanning in-bounds anchors cannot
find anything new: those anchors are already canonical. Simply replacing all
raw anchors with canonical ones in a set is unsafe: distinct raw holes may
wrap onto the same tile, and hiding one must not erase the other.

The map now tracks raw holes as before and maintains a separate derived set of
raw anchors outside the map. A toroidal lookup first checks the canonical raw
anchor and scans only the outside set. Bounded lookup still uses raw anchors.
The derived index is rebuilt on load; it is not part of the save format. When
map dimensions have changed since indexing, lookup falls back to the complete
raw scan. Mutation while dimensions differ invalidates the derived index until
all holes are removed or the map is loaded again. This avoids modifying a
cache from read-only lookups and preserves the existing synchronization model.

`MapObjectDrawOrderTest.WrappedHoleLookupPreservesOverlappingRawAnchors`
checks two distinct outside anchors mapping to one tile and hiding each in
turn. `WrappedHoleLookupSurvivesDimensionChanges` checks a hole added after a
resize, followed by a return to the original dimensions and removal. Existing
bounded, wrapped, hidden-object and Grail save/restore tests also passed.

## Bounded local measurement

On macOS arm64, Release build, one synthetic 128×128 map with 300 in-bounds
holes was queried 20,000 times for the same empty tile. Three runs before the
change took 6,499–6,758 µs for toroidal lookup and 39–41 µs for bounded lookup;
afterward toroidal lookup took 79–90 µs and bounded lookup 40–50 µs. This is a
warm, repeated microbenchmark, not a game frame, end-to-end pathfinder profile,
worst-case bound, or measurement with outside anchors. It supports improving
this particular hot path; it does not establish a user-visible speedup.

The temporary measurement test was removed from the regular suite. Its exact
patch is retained on TB5 as `runtime/hole-lookup-measurement-2026-11-14.patch`
(SHA-256 `de71f72044e8bfeceea65aff77ae9bc92ec41d41d2c48ba1700bb6014fd463ae`);
run `vcmitest --gtest_filter=MapObjectDrawOrderTest.MeasureToroidalHoleMisses
--gtest_repeat=3` with that patch applied.

The final macOS arm64 full `vcmitest` run with the prototype save, VMAP and
legacy H3M fixtures passed 1,500 tests from 226 suites without skips; four
remain disabled. `vcmitest` and `vcmiserver` built. The full-run log is
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/full-suite-hole-index-review-2026-11-14.log`.

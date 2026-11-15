# Nullkiller object graph actors and toroidal seams

`ObjectGraphCalculator` constructs temporary heroes at known objects and uses
the AI pathfinder to compute graph links. The shared path reconstruction
validated each hero through the player callback, which only knows actual map
objects. In the reproduced case it rejected the temporary heroes, so both
the ordinary adjacent-object control and the wrapped edge connection had no
graph links. This is a pre-existing AI issue, not a demonstrated regression
introduced by toroidal coordinates. `allowObjectGraph` is false for every
shipped difficulty in the current configuration, and Nullkiller also checks
whether open-map behavior is permitted; this fix does not enable that mode.

For the graph calculation only, pathfinder settings now identify the passed
temporary actors explicitly. Storage accepts those exact pointers during that
calculation; ordinary searches still use the existing player-callback check.
The graph calculator discards path storage before destroying its temporary
heroes, including when stack unwinding exits the calculation. The next normal
search constructs its own storage and does not inherit the temporary set.

On macOS arm64, the focused fixture builds the real object graph using real
resource objects and the AI pathfinder. It checks each outward edge and corner
direction on both levels, with a normal adjacent-object control on both
bounded and toroidal maps. The direct cross-edge graph link exists only on
the toroidal map in these scenarios. Each case also checks that graph storage
has been discarded, then reuses the same pathfinder for a real hero: its path
is present before removal and unavailable after the hero is removed. All 16
cases passed: `runtime/ai-objectgraph-seams-final-2026-11-15.xml` on the TB5
VCMI build root. This fixture uses revealed, synthetic test maps; it does not
exercise a full AI turn or establish strategic quality, performance, fog
confidentiality or every kind of graph link.

Before the 16-case expansion and the final fixture-lifetime cleanup, the same
implementation passed the full suite: 1,684 tests, three disabled. The XML
report is `runtime/full-suite-ai-objectgraph-2026-11-15.xml`; the eight added
underground test cases subsequently passed in the focused 16-case run. The
CMake build succeeded with pre-existing duplicate-library and missing Conan
search-path warnings.

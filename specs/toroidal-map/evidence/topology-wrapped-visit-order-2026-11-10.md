# Unstable visit order in a toroidal VMAP

Two synthetic resource objects overlap at `(35,10,0)`, with one object's
visitable cell wrapped from `(-1,10,0)`. Hiding and showing the first object
changes the tile's top visitable object. Before the guard, saving the map and
reopening it changed the selected instance from `resource_0` to `resource_1`:
the writer stores map objects in object order, and the reader reconstructs
tile occupancy in that order rather than the altered in-memory order. An
explicitly enabled regression failed on that comparison.

The writer now refuses toroidal VMAP saves when a tile's visitable list is not
in object order; the editor marks that condition as critical. The ordinary
seam-overlap warning remains. `MapFormat.ToroidalInteractiveOverlapRejectsUnstableVisitOrder`
checks that a stable overlap survives a round trip with the same top object,
that the altered order is rejected, and that this new guard does not reject
the equivalent altered overlap on a bounded map. The two external legacy map
round-trip tests also passed with the guard. Client, server, editor and test
targets built on macOS arm64; the linker reported duplicate libraries and
missing Conan search paths but completed successfully. The full suite passed
1,431 tests, skipped three optional external-input tests and reported five
disabled tests (`full-suite-visit-order-guard.log` on TB5). SpecPM validation
reported no errors or warnings.

This is a conservative save guard, not a representation of arbitrary tile
orders or a policy for choosing between overlapping interactive objects.
The editor diagnostic has been built but not exercised in its UI. An overlap
whose visit order is stable can still have a different visual top object.

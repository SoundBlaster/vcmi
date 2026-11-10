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

The regression can optionally export its stable-overlap fixture using
`VCMI_TOROID_OVERLAP_OUTPUT`; its object templates retain real resource
animations so that the fixture can also be inspected in the editor. The
focused test passed and produced a VMAP with SHA-256
`beb969e74727b41abded569c2478026db3cacb9fb28d7bc0eb1d1ff63cd122b6`.
An attempt to open it in three test editor bundles did not reach an editor
window. Process samples showed two blocked in filesystem opens, one while
loading dynamic libraries and another while opening a log file. The exact
cause was not established; in particular, this does not prove that macOS
privacy permissions caused the stall. The test editor processes were
subsequently force-quit. The diagnostic's appearance and behavior in the UI
remain unverified.

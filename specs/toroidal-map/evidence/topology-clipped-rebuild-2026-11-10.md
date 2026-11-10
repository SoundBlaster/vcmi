# Fully clipped occupancy during topology changes

In a bounded 10×10 map, a shown object with mask `["A0"]` and anchor
`(0,5,0)` has both its visitable and blocking cell outside the rectangle.
Consequently no tile index contains it. The existing default
`setToroidalAndRebuild(true)` collected only indexed object IDs and failed to
restore its occupancy at `(9,5,0)`. The previously disabled regression was
made active before the change: both the visitable and blocking assertions
failed. The map editor already selected an explicit all-objects rebuild; that
path is retained for authoring maps whose object list is entirely placed.

`showObject` and `hideObject` now track shown objects whose blocking or
visitable cells are entirely unindexed. The default rebuild collects these as
well as the normally indexed objects; it does not restore objects hidden by
`hideObject`. Tracking uses object identity so editor reindexing does not
change it. The focused test verifies a reindex, transition to toroidal,
wrapped blocking and visitable occupancy, a hidden peer remaining hidden,
a transition back to bounded, and explicitly hiding the clipped object before
a second transition.

The tracking set is in-memory state and is cleared when a game save is read.
A save containing a shown but entirely clipped object does not distinguish it
from a hidden one on later default topology changes. This change does not add
a save format or network version, and does not resolve that case. Normal map
format loading and the editor's explicit all-objects authoring path remain
separate from this save-game limitation. No GUI verification of this change
was performed.

On macOS arm64, source `ca955bd` plus this change built `vcmitest` on TB5.
The focused related run passed 20 tests, including wrapped VMAP round-trip,
overlap policy and boat movement. After adding reindex coverage, the updated
regression passed and the full `./vcmitest` run passed 1,455 tests; three
optional-input tests were skipped and four were disabled. The full log is
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/full-suite-unindexed-occupancy-2026-11-10.log`.
Linker duplicate-library and missing-search-path warnings were present.

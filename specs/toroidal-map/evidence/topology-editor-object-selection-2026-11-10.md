# Editor selection and movement preview at map seams

The editor previously invalidated and painted only an object's unwrapped
rectangle. A selected seam-crossing object could consequently lack a selection
outline on the opposite side, leave stale pixels when selected or locked, or
show only part of its movement preview.

The viewport now finds periodic copies of each object rectangle that intersect
the canonical map area. It uses these copies when invalidating object sectors,
drawing the selection outline and preview, and updating the old and new
preview positions. Bounded maps retain one original rectangle. The object
itself remains a single map instance; these copies are presentation geometry.

`MapTopology.EditorObjectCopiesCoverTheCanonicalAxis` compares copy coverage
and multiplicity with independently enumerated wrapped positions for axis
sizes 3–9, lengths up to two periods and starts up to two periods in either
direction. Five focused topology, object occupancy and VMAP tests passed.
The macOS arm64 editor and test targets built successfully; the linker
reported existing duplicate-library and missing Conan search-path warnings.
The coordinate helper is tested, but Qt painting, mouse movement and redraw
in a live editor window have not been verified.

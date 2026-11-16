# Editor minimap: toroidal reference and viewport

The editor minimap now draws a cosmetic dashed cyan outline around the
canonical map rectangle for toroidal maps. This outline is a coordinate
reference, not a movement barrier. Its viewport layer divides a toroidal
viewport into canonical rectangles at either seam and at corners, including
when the viewport covers a full map period. Bounded maps retain the previous
viewport rendering and do not draw the new outline.

On macOS arm64, the `vcmieditor` and `vcmitest` targets built from `0c82618`
plus this change. Four focused topology tests passed. The new test independently
enumerates the visible canonical cells for ordinary, edge, corner, negative
and larger-than-map rectangles. The build emitted existing duplicate-library
and missing Conan search-path linker warnings. This tests geometry and build,
not the appearance of the cyan outline or its contrast in the actual editor.

Continuous toroidal editor scrolling is not implemented by this change.
The viewport layers still clamp sectors to one map period, and repeated-view
input and editor gestures need their own implementation and graphical checks.

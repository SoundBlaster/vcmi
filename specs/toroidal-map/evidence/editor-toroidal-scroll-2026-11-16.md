# Initial toroidal editor navigation check

On macOS arm64, the editor and `vcmitest` built from `d8baaa2`'s changes
(the binary was built before that commit was created). The `MapTopology.Editor*`
and `MapManager.*` filters passed 15 tests. The test used an isolated profile
and a copy of the 36 by 36 toroidal seam laboratory map on TB5.

Computer Use accessed the loaded editor window. Clicking near a minimap corner
displayed map content across both seams and split the white viewport outline
across both pairs of minimap edges. The cyan rectangle marked the original
coordinate reference. Repeated left-arrow input moved the view and minimap
viewport. In the displayed repeated portion, the inspector identified a town;
a 2 by 2 selection painted snow. The map validator warned about errors in the
synthetic fixture when saving. The saved map archive nevertheless contained
the four changed snow cells at canonical coordinates (33–34, 11–12) and
surrounding terrain transition changes; other archive members were unchanged.
The same copied map was reopened in a fresh editor process and the snow patch
was visible. Starting near the minimap corner, repeated up-arrow input moved
the viewport across the north seam; the minimap viewport moved accordingly.
Another 2 by 2 snow edit on a repeated portion was visible, then disappeared
from the canvas and minimap after Undo. Redo was not exercised.

On a later interactive check with the same build, a gold resource was selected
at (35, 35), dragged across both seams and saved. Inspection of the VMAP
archive confirmed `resource_4` at (1, 2). Reopening the saved map showed the
resource at the new location. Near the north-west corner, alternating Left
and Up moved the camera to the opposite corner on the minimap; observations
were discrete and do not establish smoothness. The editor then opened an
independent 36 by 36 bounded control with no topology field: repeated Left
and Up at its north-west limit and Right and Down at its south-east limit did
not wrap its viewport, which remained one rectangle on the minimap.

An attempt to drag a new decoration from the object browser onto a repeated
portion did not create an object; the automation gesture selected the browser
item instead. This does not distinguish an input automation limitation from an
editor issue, and object creation on a repeated portion remains unverified.

The earlier claim that the editor window was unavailable to Computer Use was
based on observing an incomplete startup, not a demonstrated incompatibility.
After the interactive session, a subsequent launch stalled in the macOS dynamic
loader while opening a file; this does not establish an editor deadlock.

These are discrete observations, not a continuous video or proof of smooth
animation. Other corners and directions, object placement and multi-tile
object moves, selection variants, redo and broader bounded-map behavior still
require verification. This record does not establish full
conformance to `editor_toroidal_navigation`.

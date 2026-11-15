# Mixed roads across seams and a corner

`MapFormat.ToroidalMixedRoadTurnsRoundTrip` builds a separate 36×36 toroidal
map. It paints a dirt-to-gravel transition across the east-west seam, a
gravel-to-cobblestone transition across the north-south seam, and a
cobblestone turn at `(0,0)` whose northern arm crosses the north-south seam.
Each shape has an interior control with the same road types. The test compares
the sprite group and reflection against the control, then saves and reloads
the VMAP and checks each road tile's type, exact sprite and flags. It removes
test-only registry entities before writing the fixture. The optional
`VCMI_TOROID_MIXED_ROAD_OUTPUT` refuses to overwrite an existing file.

On macOS arm64 the test passed alongside the earlier visual-fixture round trip
and edge-turn brush test. It also passed alongside the exhaustive 256-neighbor
brush comparison. Results are `runtime/mixed-road-turns-2026-11-15.xml` and
`runtime/mixed-road-neighborhoods-2026-11-15.xml` under the TB5 build root.
The build reported duplicate-library and missing Conan search-path warnings.

The generated VMAP was opened by the signed graphical client stamped
`1.8.0.46f05d2`, with the same binary used in the
[mixed-version check](topology-old-network-peer-2026-11-15.md). In an unsaved
single-player session, `vcmimap` revealed the map. At one observed camera
position, both mixed-surface seam transitions and the corner turn appeared
connected. The hero was moved away from the turn to expose it. The client
exited with code 0 and port 3030 was no longer listening. No save or reload
was made. The screenshot
was observed but not retained; the client log and observations are under
`runtime/gui-mixed-road-turns-2026-11-15/` on TB5.

The map SHA-256 is
`d718cc46b250cb757b778bba12be624198cef563165b36ee7659f85772f569ed`.
The generated archive contains no copied game artwork. This checks specific
surface positions and one static view, not every artwork arrangement, Qt
editing, the underground layer, or frame continuity during scrolling.

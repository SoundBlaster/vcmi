# Wrapped static footprints in VMAP

The VMAP reader and writer no longer reject every static object that crosses
a toroidal seam. They still reject static footprints wider or taller than the
map period: occupancy can deduplicate such footprints, but the renderer does
not represent multiple image slices from the same object on one canonical tile.
Bounded maps retain their previous behavior. The editor reports oversized
footprints as critical and warns when two static visitable objects share a
tile and one crosses a seam. That warning is not a selection policy; its UI
behavior has not yet been exercised.

`MapFormat.ToroidalWrappedFootprintsSurviveVmapRoundTrip` constructs a
two-level legacy fixture and opts it into toroidal topology using the editor's
all-objects rebuild. A resource's wrapped blocking and visitable cell overlaps
a static obstacle; a second obstacle crosses an underground corner; and a
third has its entire anchor outside the old rectangle. Saving and reopening
VMAP retains the objects and the checked occupancy on the correct level. The
test also verifies that the writer and reader reject an oversized static
footprint on toroidal maps, while the same template loads on a bounded map.
These are synthetic templates and do not establish general legacy conversion.

`ToroidalDemoVmapTest.SavedWrappedResourceCanBeCollected` serializes a
seam-crossing resource, initializes a real game state from the resulting VMAP
and uses the server movement handler to collect it from the opposite side.
This does not verify all object types or the behavior of overlapping
interactive objects. The existing draw-order test shows that the visually
top object and the selected visitable object can differ in such an overlap.

On macOS arm64 the client, server, editor and test targets built. Sixteen
focused tests passed. After adding the oversized-reader check and the visual
town fixture, the full suite passed 1,430 tests, skipped two optional
external-prototype tests and reported five disabled tests
(`full-suite-wrapped-footprints-final.log` on TB5). Linker warnings reported
duplicate libraries and missing Conan search paths despite successful links.

## Computer Use: real town sprite

`MapFormat.ToroidalVisualWrappedTownFixtureRoundTrips` places a normal Castle
town at `(0,4,0)`, checks that its footprint crosses the horizontal seam and
saves and reloads the fixture. On the same day a diagnostic client started the
saved `Toroid Wrapped Town` scenario using local temporary copies of the
Complete game archives. Its build label was `1.8.0.caa22dd`; the client
executable SHA-256 was
`35639d7f848062ec3e5d8b0a3ef49cd62bdb88daa3be355b1de3e3fec2e98077`,
the bundled library SHA-256 was
`9b53534297a64c5c7ff1350d1025fd83c9c4bc4c239d4973f128faaa2a17a2ae`,
and the VMAP SHA-256 was
`426b9efb37c2b8a1c7c5b7d15485d3c169451b043336a13b9b5d1593a9900339`.
The label names the base commit; the library includes this uncommitted change.

The game began successfully. In the initial view, and after revealing the map
and scrolling west for 250 ms then east for 500 ms, the blue town appeared as
one complete sprite without an obvious seam gap at the observed positions.
This is a visual sample, not continuous frame capture, a click/visit test, or
a proof for other object types or overlapping graphics. The unsaved session
was closed, temporary game-data copies removed, and the diagnostic profile's
TB5 links restored. The user's installation and source archives were not
changed.

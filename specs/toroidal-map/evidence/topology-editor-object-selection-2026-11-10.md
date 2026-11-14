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

## Attempted live check

A test editor bundle with the current object-selection code was signed with a
development identity and launched through Computer Use. No editor window
appeared. Activity Monitor sampled the main thread in
`CBasicLogConfigurator::configureDefault` while opening a log file. A second
diagnostic build changed only the editor log filename to a separate file on
the same volume; it likewise did not reach a window and its sample stopped at
the same logging call. This weakens the hypothesis that both editors merely
contended for the same filename; it does not establish the cause or prove a
macOS privacy-permission failure. Both test processes were force-quit and the
temporary source edit to the log filename was reverted. The pre-existing
editor and its unsaved map were left open. No visual selection or preview
result was obtained.

A further diagnostic build sent the editor log to `/dev/null`, rather than
opening a file on TB5. It passed logging but still did not show a window; the
Activity Monitor sample found the main thread in
`initializeBaseFilesystem` / `CResourceHandler::initialize`. This indicates
that avoiding the log-file open alone is insufficient. The sample does not
identify the filesystem operation or establish why it stalled. The probe was
force-quit, the temporary source edit was reverted, and the ordinary editor
target was rebuilt from the restored source.

## Graphical seam selection and saved copy

On 2026-11-15, a signed macOS arm64 editor built from `105f539` opened a
separate copy of `toroid-seam-laboratory.vmap` with the user's Complete data.
The source VMAP had SHA-256
`3fbc0a2cbbdfa22068e7e6a099a7b51e803d39b5fc0d0b3fd95155cc6eeb232d`.
Dragging the red town `town_0` toward the west edge produced visible parts
on both horizontal edges of the map and minimap. While the town was selected,
its white selection outline appeared over both visible parts. Deselecting it
and clicking the opposite-edge part selected the same `town_0` in Inspector.
Dragging that part one tile moved the town, and Undo restored the prior view.
The modified copy was saved and reopened; both parts and the opposite-side
outline appeared again. This is one town and one horizontal seam, not a
continuous drag-preview, vertical/corner, multiplayer, or gameplay test.

The copy at `runtime/editor-seam-check.roNFmO/seam-laboratory.vmap` under the
TB5 build root was subsequently modified for an Inspector test described in
[the separate record](editor-inspector-dirty-state-2026-11-15.md). It is a
test artifact and is not the playable demo. The original VMAP remained
unchanged. A different initial wrapped-town fixture had missing test-mod
identifiers and did not produce a usable editor window in this attempt.

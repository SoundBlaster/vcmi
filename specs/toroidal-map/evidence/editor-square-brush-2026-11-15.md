# Square editor brushes at toroidal edges

The editor's 2×2 and 4×4 terrain selection brushes previously discarded
offsets outside the map rectangle even on a toroidal map. The button-press
and drag paths now map those offsets to canonical cells on the same level,
for both selection and erasure. Duplicate cells on maps smaller than a brush
are returned once, and an outside tile coordinate does not paint a different
side. On bounded maps the brush still clips its offsets, including when its
center tile is just outside the map. The same selection is used before
applying terrain, roads or rivers.

Separately, the line tool's right-button drag passed the previous preview
instead of the newly calculated cells to `erase`. The call now uses the new
cells. This affects bounded and toroidal maps and is a source-level fix;
the actual mouse gesture has not been exercised in this check.

On macOS arm64, `vcmieditor` and `vcmitest` built from `5b8d6be` plus these
changes. `MapTopology.EditorSquareBrushWrapsOnlyOnToroidalMaps` checks both
brush sizes at four corners and an interior tile on both levels against an
independent expected-cell calculation. It also covers bounded clipping,
outside pointers, invalid levels and duplicate suppression on a 3×3 map.
`MapManager.SquareBrushSelectionPaintsAndUndoesRoadAcrossCorner` passes the
2×2 corner selection to the real map edit manager, verifies road tiles and
undo/redo on toroidal and bounded maps. The four editor topology tests, eight
`MapManager` terrain/road/river tests and the Crossroads VMAP round trip
passed (13 tests total). The report is
`runtime/editor-square-brush-2026-11-15.xml` under the TB5 build root.

The geometry test exercises the helper used by the input handlers, not Qt
mouse events or rendered selection. Neither a live editor gesture nor a
saved map painted using this new gesture has been verified. The linker
reported duplicate libraries and absent Conan search paths, but the build
completed successfully.

## Attempted graphical check

A separately signed test editor was built from `11fc869` plus the editor log
override, and launched with a copy of `toroid-crossroads.vmap`. The log shows
the copy loading, and a process sample placed the main thread in the Qt event
loop. Computer Use could access the editor's File menu but reported an empty
window without the map or controls, including after a raise and zoom attempt.
The cause was not established. No brush gesture was performed and no GUI
behavior is claimed. The application quit normally. Source and copy both
retained SHA-256 `dc22f9087595505366d7e6c6ff585df4e9b265436a18a2c7d6d539493848735f`.

The process wrote its log to the explicitly supplied
`runtime/editor-brush-11fc869/editor.log`, not the normal shared editor log.
This did not isolate editor preferences: opening the copy updated the recent
files list, and the window zoom attempt may have changed the saved window
geometry. The original preferences were not captured before the run.

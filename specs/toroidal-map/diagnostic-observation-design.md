# Experimental invisible observation layer

## Intent and boundary

Provide a small, opt-in diagnostic representation of the player's current
adventure-map view and GUI. It should help a test correlate canonical entities,
their repeated screen copies, camera geometry and GUI controls without relying
on OCR. This is a local debugging facility for the toroidal experiment, not a
gameplay rule, accessibility implementation, remote-control protocol or ASP.
It must not alter rendering, input or game state.

The first vertical slice is a bounded trace of the adventure map view: record
the camera, level, viewport, map period and visible tile/object copies when
observed state changes. Store raw displayed and canonical tile coordinates
separately. An object may appear in several displayed copies but still have one
game identity. The trace is produced only when explicitly enabled and written
to a caller-selected local path with a finite snapshot budget. Normal play does
not create a trace or scan the viewport.

Prototype: set `VCMI_MAP_OBSERVATION_PATH` to a writable local file before
launch. The client appends at most 1,800 JSON Lines per process; failure to
open the file leaves observation disabled. Each line
contains a session identifier, sequence, monotonic timestamp, level, map and
viewport geometry, camera center and candidate object copies. Each candidate
contains the object ID, displayed tile, canonical tile, screen-space tile
rectangle in logical GUI coordinates and native image offset. A candidate is
a renderer input, not a
guarantee that pixels were visible or that the object was clickable. At most
20,000 viewport tiles and 512 candidates are examined/emitted per record;
`complete: false` marks truncation. An explicit path enables local writes;
the game appends, never truncates an existing trace. No environment variable
means no file and no viewport scan.

The subsequent prototype revision can include the top window's widget tree
when that window derives from `CIntObject`. Nodes have per-record indices and
parent indices, logical rectangles, active/disabled flags, and a coarse
`button` or `widget` kind. The client inspects map state at most every 100 ms,
and checks the tree at most every 200 ms or when it observes a top-window
change; it stores the last tree in emitted snapshots.
`gui_observed_ns` gives its own monotonic observation time; a brief change
between checks may be missed, as may brief map changes. Traversal is capped at 4,096 nodes and depth
32; `gui_complete: false` marks a truncated tree. The absence of a
compatible top window is reported by `gui_available: false`. These values
describe in-memory objects at their observation time, not occlusion, painted pixels,
stable control IDs, semantic names, or guaranteed ability to click. The trace
does not yet capture GUI-only frames when the adventure map is not shown.

Use the active renderer context and its visibility check; do not enumerate the
authoritative map or reveal fog-hidden entities. A diagnostic view must be no
more privileged than the view being recorded. Record geometry and identifiers,
not names, chat or save contents. The first slice does not create a network
endpoint or accept actions. File-access errors must not crash the game.

Later slices can expose stable semantic roles and actions. `WindowHandler`
keeps the window stack, while `CIntObject` has child
links, rectangles and disabled state, but that alone does not prove occlusion,
actionability, semantic names or what was actually painted. Those fields need
explicit contracts and focused checks before consumers rely on them.
`VisualLogger` already accepts map lines and text for an on-screen overlay;
it is not a complete entity/GUI snapshot and need not be repurposed.

## Evidence and limits

Version 2 emits an initial snapshot, then only when the level, camera tile,
viewport/zoom, visible object-copy set or observed GUI tree changes. After
the camera stops moving it emits its exact final position once. Sub-tile
camera movement and sprite animation alone do not cause snapshots; each
emitted record retains precise geometry at its own observation time. The
`show_count` field identifies how many map-show calls have been examined.

Snapshots are observations of renderer inputs, not proof of pixels presented
by the OS or a replacement for screenshots/video. Sequence numbers and
monotonic timestamps help order recorded changes; pixel gaps, sprite clipping, flicker and perceived
smoothness still require actual rendering evidence. Identify the source/build,
backend, fixture and trace when reporting a test. The feature remains
experimental until a real client session and focused tests verify it.

## Planned steps

1. Prototype the bounded, opt-in adventure-map trace and unit-test canonical
   copy mapping, hidden tiles, bounded maps and the viewport budget. The
   20-test standalone topology suite passed on macOS arm64; the client target
   built. JSONL output was exercised in one local client session.
2. A signed client session produced 1,800 records including both seam
   crossings and an independent screenshot; see [the observation record](evidence/map-observation-client-2026-11-10.md).
   Further synchronized visual comparison and broader fog cases remain open.
3. Prototype window/widget geometry. The standalone topology suite checks the
   bounded tree walk and parent indices. The initial 256-node, every-frame
   version truncated every tree and wrote 53 MB in one session. The
   [change-driven revision](evidence/observation-on-change-client-2026-11-10.md)
   produced 24 snapshots in a separate client session; its world-view object
   enumeration hit the former 4,096-tile limit. The larger limit and the
   bounded inspection frequency still require a new client-session check.
   Define stable semantics and actionable roles only where the
   game supplies them. Consider a future protocol adapter separately.

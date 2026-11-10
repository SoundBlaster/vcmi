# Experimental invisible observation layer

## Intent and boundary

Provide a small, opt-in diagnostic representation of the player's current
adventure-map view and GUI. It should help a test correlate canonical entities,
their repeated screen copies, camera geometry and GUI controls without relying
on OCR. This is a local debugging facility for the toroidal experiment, not a
gameplay rule, accessibility implementation, remote-control protocol or ASP.
It must not alter rendering, input or game state.

The first vertical slice is a bounded trace of the adventure map view: record
the camera, level, viewport, map period and visible tile/object copies on
successive map render calls. Store raw displayed and canonical tile coordinates
separately. An object may appear in several displayed copies but still have one
game identity. The trace is produced only when explicitly enabled and written
to a caller-selected local path with a finite frame budget. Normal play does
not create a trace or scan the viewport.

Prototype: set `VCMI_MAP_OBSERVATION_PATH` to a writable local file before
launch. The client appends JSON Lines for at most 1,800 `MapView::show` calls
per process; failure to open the file leaves observation disabled. Each line
contains a session identifier, sequence, monotonic timestamp, level, map and
viewport geometry, camera center and candidate object copies. Each candidate
contains the object ID, displayed tile, canonical tile, screen-space tile
rectangle and native image offset. A candidate is a renderer input, not a
guarantee that pixels were visible or that the object was clickable. At most
4,096 viewport tiles and 512 candidates are examined/emitted per record;
`complete: false` marks truncation. An explicit path enables local writes;
the game appends, never truncates an existing trace. No environment variable
means no file and no viewport scan.

Use the active renderer context and its visibility check; do not enumerate the
authoritative map or reveal fog-hidden entities. A diagnostic view must be no
more privileged than the view being recorded. Record geometry and identifiers,
not names, chat or save contents. The first slice does not create a network
endpoint or accept actions. File-access errors must not crash the game.

Later slices can expose a structured GUI tree and selected semantic roles and
actions. `WindowHandler` keeps the window stack, while `CIntObject` has child
links, rectangles and disabled state, but that alone does not prove occlusion,
actionability, semantic names or what was actually painted. Those fields need
explicit contracts and focused checks before consumers rely on them.
`VisualLogger` already accepts map lines and text for an on-screen overlay;
it is not a complete entity/GUI snapshot and need not be repurposed.

## Evidence and limits

Frame records are observations of the renderer's inputs and timing, not proof
of pixels presented by the OS or a replacement for screenshots/video. Frame
sequence numbers and monotonic timestamps can reveal missing records and
camera discontinuities; pixel gaps, sprite clipping, flicker and perceived
smoothness still require actual rendering evidence. Identify the source/build,
backend, fixture and trace when reporting a test. The feature remains
experimental until a real client session and focused tests verify it.

## Planned steps

1. Prototype the bounded, opt-in adventure-map trace and unit-test canonical
   copy mapping, hidden tiles, bounded maps and the viewport budget. The
   20-test standalone topology suite passed on macOS arm64; the client target
   built. End-to-end output and real-client fog behavior remain to verify.
2. Exercise it in a signed local client on the seam laboratory map and compare
   consecutive camera/object positions with independent screenshots.
3. Extend GUI observation incrementally, starting with window/widget geometry;
   define stable semantics and actionable roles only where the game supplies
   them. Consider a future protocol adapter separately.

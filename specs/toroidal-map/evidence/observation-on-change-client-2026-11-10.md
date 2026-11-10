# Change-driven observation in a running client

On macOS arm64 a development-signed client built from commit `1948e6e` was
launched with the local observation path enabled and loaded the same isolated
36×36 Toroid Seam Laboratory save. While the map remained stationary, two
separate file inspections showed one snapshot. After Ctrl+Right moved the
camera and the player opened and closed world view, the client was closed
without saving.

The final trace contained 24 parseable, consecutively numbered snapshots in
826,364 bytes. The first and second recorded snapshots were separated by
28.442 seconds and 3,264 examined map-show calls; no additional snapshot was
written in that interval. The first observed screen was stationary; the next
record followed camera input. All 24 widget trees contained 303 nodes, with
valid parent indices and `gui_complete: true`. The two world-view snapshots
used 7×7-pixel tiles and reported `complete: false`: the fixed viewport-tile
budget was reached, so their object-candidate lists are not complete. The
ordinary 32×32 snapshots in this run did not report that truncation.

These observations verify the listed idle, camera and view-switch behavior
for this session. GUI is sampled at most every 200 ms unless the top window
changes, so transient changes can be missed. The trace excludes hidden tiles
according to the player callback, but this run is not a complete fog audit.
The trace is a sampled model of renderer inputs and widgets, not a recording
of presented frames or proof of seamless animation. It contains no semantic
button labels or control actions.

The local TB5 artifact is
`runtime/observation-on-change-1948e6e.jsonl`, SHA-256
`461f950cbbde86da968292686459e2c51d53a438abb43f51e00fd49446c62b7b`.
The signed client's SHA-256 is
`d867be195d8d28ce464ca17cf8bd4aee27a1e8358cceede36fec1319973af21f`.
The focused topology suite passed 22 tests; build and test logs are
`observation-on-change-build.log` and `observation-on-change-tests.log` in
the TB5 build root.

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

## Bounded inspection and wider world view

The later signed client from `28348ef` used a 100 ms minimum interval between
map inspections and a 20,000-tile scan limit. On the same save, Computer Use
opened world view, returned to the map and scrolled right before quitting
without saving. The trace held 12 snapshots across show counts 0 through
2,678, totaling 482,969 bytes. The smallest interval between consecutive
snapshot timestamps was 100.15 ms. All emitted records reported both map and
GUI enumeration complete. At the 7×7 world-view scale the largest snapshot
contained 440 candidate tile/object pairs and 303 GUI nodes. This establishes
the limit was sufficient for this viewport and fixture, not for every zoom,
map or object density. The diagnostic overhead itself was not benchmarked.

The trace is `runtime/observation-budget-28348ef.jsonl`, SHA-256
`f9d1e28e7f03d87367bd5cb6fc2d9e17084c99827ff08a0a68566fc56b83e5e7`;
the signed client SHA-256 is
`cdfc5ae038e7fccde60463d71f23a35fcb383e107ef5f3b9acd9942320a039ce`.
The standalone topology suite passed 23 tests; see
`observation-budget-build.log` and `observation-budget-tests.log` under the
TB5 build root. Changes shorter than the inspection interval can be missed.

## Player resource and hero state

The signed client from `c2457bd` loaded the same save with an explicit trace
path. Its first snapshot recorded red player's 21,000 gold, 27 ore and the
selected hero at `(35,35,0)` with zero movement points. After ending the turn,
the GUI showed day two and 21,500 gold; a later snapshot recorded 21,500 gold
and 1,560 movement points. Two eastward keypad actions then crossed the seam
and collected ore. The GUI showed a five-ore pickup and 32 ore; snapshots
recorded the hero at `(0,35,0)`, then 32 ore and 1,360 remaining movement.
The session was closed without saving. It produced 11 complete snapshots
totaling 350,391 bytes. This checks one owned hero and resource change, not
all player state, UI changes, multiplayer confidentiality or agent access.

The trace is `runtime/observation-player-c2457bd.jsonl`. The SHA-256 of its
first 11 complete lines, corresponding to this run, is
`0a94c8677f43a0af1481fa529eaa534731679a5bb25b5c0659750fcdbb90439c`.
Later activity appended more observations to the same file, including the
[Screener integration check](screener-vcmi-window-2026-11-10.md), so its
whole-file digest differs. The signed client SHA-256 is
`6fc7ba9da0c413e1c88d63ca7ba4454b6329eec7e7c4bf8fabe4dd9c047f4756`.

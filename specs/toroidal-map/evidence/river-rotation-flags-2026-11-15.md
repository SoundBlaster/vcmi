# River redraw preserves tile flags

When the editor recalculated a river sprite, its mask retained the previous
river flip bits and cleared the coastal and favorable-winds bits instead.
Consequently, repainting could leave the river with the wrong orientation and
could change unrelated terrain behavior. This applies to bounded maps as well
as toroidal maps; it is not evidence that any particular map was affected.

The redraw now replaces only bits 2 and 3, preserving the terrain and road
flip bits and the two terrain flags. A regression test sets all eight bits,
draws an isolated river, and checks the expected unflipped sprite and retained
flags on both a bounded interior tile and a toroidal corner tile. Before the
fix the test observed `0x3f` instead of `0xf3` in both cases. After the fix,
the regression, editor map-operation tests, road redraw geometry and visual
seam fixture tests passed together (seven tests). This checks the stored flags,
not a GUI redraw or every river sprite orientation.

The macOS arm64 test, client, server and editor targets built on TB5. Logs
`runtime/river-flags-before-test.log` and `runtime/river-flags-after-build.log`
are in the checkout's TB5 build root. With the prototype map and save plus
the Complete H3M fixture supplied, the complete local suite passed 1,662 tests;
four were disabled. Its log is `runtime/full-suite-river-flags-2026-11-15.log`.

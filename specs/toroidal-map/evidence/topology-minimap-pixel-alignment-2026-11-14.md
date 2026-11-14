# Minimap click and displayed pixels

On a toroidal minimap, the prior click mapping used the left edge of the
display pixel, while nearest-neighbor rendering samples near its center. For
a 36-tile map displayed 150 pixels wide, pixel 4 displayed tile 1 but selected
tile 0. On a 252-tile map displayed 144 pixels wide, the last pixel displayed
tile 251 but selected tile 250.

An initial correction to center sampling still assumed a uniform tie rule.
Experiments showed that SDL3 software scaling and the Metal renderer can
choose different source tiles at exact boundaries. For example, after that
correction, a 36/150 minimap displayed tile 9 at pixel 37 on Metal but tile 8
on the SDL software path. A software sweep of map sizes 3–252 and display
sizes 1–150 found 634 size pairs where that simple tie rule did not match
SDL's scaler. The initial change therefore did not establish correctness.

Toroidal minimaps now construct a logical-resolution raster directly from the
same pixel-to-tile mapping used for clicks. Each map tile owns a calculated
half-open display interval; refreshing fog or an object's ownership repaints
the affected interval. The raster is then drawn at the widget's size. Bounded
maps retain their original path. When a map has more tiles than display pixels,
not every tile can have its own distinct minimap pixel.

`MapTopology.ToroidalMinimapTileIntervalsPartitionDisplayPixels` checks that
intervals cover each pixel exactly once and agree with clicks over all 37,500
combinations of map extents 3–252 and display extents 1–150. Two standalone
SDL tests paint source tile colors through those intervals and check every
display pixel through a software canvas and a software-renderer texture path
for ten selected sizes. The earlier direct-scaling tests failed before the
correction. The full standalone SDL suite passed four tests, and the topology
suite passed 27 tests on macOS arm64.

An independent [SDL3 probe](../tools/minimap-metal-probe.c) created a hidden
Metal renderer and copied a precomputed logical raster to a render target at
twice its width. Readback matched the expected tile for every physical pixel
in seven selected size pairs, including 36/150 and 252/144. This tests the
copy and integer upscaling, not the VCMI GUI or every Metal device. The full
local `vcmitest` suite passed 1,536 tests without skips; four remain disabled.
Logs on TB5:
`runtime/minimap-renderer-independent-build.log`,
`runtime/minimap-renderer-independent-sdl.log` and
`runtime/minimap-renderer-independent-topology.log`, and
`runtime/full-suite-minimap-raster-2026-11-14.log`.

## Interactive check on the updated client

The clean `fae12b1` client was built on TB5, copied into the existing signed
preview app, had its library references rewritten to the bundled frameworks,
and was re-signed and verified. The separately bundled engine library and
server remain from `961e235`; the only changes to client, server and library
source between those revisions are the minimap changes. The window title is
therefore still `1.8.0.961e235`, not the client source revision. The updated
client's post-signing SHA-256 is
`840b06033f3b870bae9e331bf161c29049b7d9d51db040cb2b9e7b8bb656858d`;
the bundled library remains
`59aa0556c0a62c8a5e0100c65d63714792f895d58742f09fdf6515ce7bfd9d68`.
It started and loaded the `Toroid Open Water Corner` test save without
additional user intervention.

On its 36×36 map and 144×144 logical minimap, diagnostic clicks on the four
inside corner pixels centered the camera on tiles `(0,0)`, `(35,0)`, `(0,35)`
and `(35,35)`, respectively, as recorded by the observation stream. Computer
Use showed the final map after selection. The 11-record local trace is
`runtime/observation-minimap-fae12b1-2026-11-14.jsonl` on TB5, SHA-256
`81ca6ddbeb94390ddd1446119ce350586b028573e99e469225a767c4156bff78`.
This GUI check uses an integer 4:1 minimap scale; it does not demonstrate
the original fractional-scale mismatch, all terrain colors, fog updates, or
animation continuity. SDL2, noninteger compositor scaling and other GPU
backends remain unverified.

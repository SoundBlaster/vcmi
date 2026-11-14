# Minimap click and scaled-pixel alignment

On a toroidal minimap, the previous click mapping used the left edge of the
display pixel. SDL3's nearest-neighbor scaler samples at the pixel center.
For a 36-tile map displayed 150 pixels wide, pixel 4 displayed tile 1 but
selected tile 0. On a 252-tile map displayed 144 pixels wide, the last pixel
displayed tile 251 but selected tile 250.

The toroidal mapping now uses the center sample, choosing the earlier tile at
an exact sample boundary as observed in the SDL software renderer. Bounded map
mapping is unchanged. A larger map necessarily has more tiles than display
pixels, so not every tile can have its own distinct minimap pixel.

`WrappedSpriteSdl.ToroidalMinimapClickMatchesScaledSourcePixel` paints source
tiles with distinct colors and checks every output pixel against click mapping.
`ToroidalMinimapClickMatchesTextureScaledSourcePixel` repeats the check via
the texture path with SDL's software renderer. Both failed before the fix and
passed after it for ten source/display sizes, including 36/150, 144/73 and
252/144. The full standalone SDL suite passed four tests and the standalone
topology suite passed 26 tests. The `vcmiclient` target built on TB5; existing
linker warnings remained. Logs are `runtime/minimap-scaling-before-2026-11-14.log`,
`runtime/minimap-sdl-suite-2026-11-14.log` and
`runtime/minimap-topology-suite-2026-11-14.log`.
The complete local `vcmitest` suite also passed 1,534 tests without skips;
four remain disabled (`runtime/full-suite-minimap-alignment-2026-11-14.log`).

These checks use the SDL3 software renderer on macOS, including its texture
path. A hardware renderer, SDL2, scaling by a different window/compositor and
an interactive click in the updated GUI have not been verified.

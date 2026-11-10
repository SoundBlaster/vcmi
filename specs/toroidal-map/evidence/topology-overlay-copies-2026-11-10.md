# Overlay coordinates on repeated map copies

`MapViewCache` previously passed the displayed, unwrapped tile coordinate
directly to image and text overlays. Spell-view icons compare that coordinate
to canonical object positions; the world-view icon lookup also checks an
object's visitable mask at the supplied coordinate. A repeated copy outside
the rectangle could therefore omit an icon. Text labels used the displayed
x-coordinate to choose their vertical offset: on a map with an odd width,
the same label could change offset when the camera crossed a seam.

The cache now maps each displayed tile to the canonical tile before asking
for overlay content, and uses that same x-coordinate for label placement.
It still places the resulting overlay at the displayed tile's screen position.
Bounded out-of-map coordinates retain their previous value. This applies to
the existing software render path and to the GPU path when overlays require
their own layer.

`RepeatedCopiesUseTheSameOverlayTileAndLabelParity` checks several horizontal,
vertical, and corner copies on a 5×7 two-level torus, including multiple
periods, invalid level and bounded behavior. `WorldViewObjectQueryUsesCanonicalOverlayTile`
checks a real object mask at repeated positions: the raw displayed tile is
not visitable, while the mapped tile is. These focused checks exercise the
coordinate mapping and object query; they do not instantiate the complete
renderer or establish visual continuity in a running game.

On macOS arm64, the client, main test target and standalone topology target
built. Nineteen topology tests and fifteen map draw-order tests passed; the
draw-order suite also reported one existing disabled regression. Logs are
`overlay-copies-build.log`, `overlay-copies-tests.log`,
`overlay-object-build.log`, and `overlay-object-tests.log` under the TB5
build root. Actual world-view and spell-view icons, label placement in the
GUI, other overlays, and continuous frames remain unverified.

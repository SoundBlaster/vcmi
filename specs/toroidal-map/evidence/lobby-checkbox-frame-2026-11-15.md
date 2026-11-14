# Lobby checkbox frame regression

The graphical loopback test exposed `accessing unavailable frame 0:2 in
CAnimation` during lobby updates. The lobby checkbox animations have two
frames, but the default disabled button mapping requests frame 2. The failed
request previously left the displayed frame unchanged; the button's input was
already blocked separately.

`ButtonBase::update` now keeps that existing image when a blocked two-frame
button maps to a missing frame. A valid custom disabled-frame mapping continues
to be used. This does not add disabled artwork or change input blocking.

## Local verification

On macOS arm64, using a signed graphical client based on `e6e538f` with the
change in `client/widgets/Buttons.cpp` (SHA-256
`9fde6bad2288d282ab9ed915e92db3d48f6024dcc3ad6542ce2258455f9b2735`):

- An initial candidate joined an unmodified `30a5dc1` host on loopback. The
  candidate guest log contained no unavailable-frame errors; the host emitted
  one during the connection at 23:52:25 on November 14. That candidate used a
  broader two-frame fallback than the final code.
- The final, narrower code was rebuilt and used to host a load-game lobby for
  the toroidal `Autosave-111`. Opening Extra Options and attempting to toggle
  the disabled recording option produced no button action or
  `LobbySetExtraOptions` packet. Toggling the enabled cheats option produced
  the packet; a second click restored it. No unavailable-frame error appeared
  in that client log. The save was selected but the game was not started.
- The test lobby was closed and TCP port 3030 was no longer listening. The
  CMake `vcmiclient` target built successfully, with pre-existing linker
  warnings about duplicate libraries and missing search paths.

The GUI test covers the default checkbox mapping and these lobby states; it
does not exercise every custom button mapping or establish visual animation
continuity. The private raw logs remain on TB5 under
`Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/gui-multiplayer-30a5dc1.iMinul/`:

| File | SHA-256 |
| --- | --- |
| `checkbox-baseline-host.log` | `fa898e8371f8f5d68a4ff78cc88c3ec7d7f33fbc41a490c273a2bf169ca2413f` |
| `checkbox-fix-initial.log` | `1263ea18b6556dd52d43be1719a86ce3623a4f41eb09b5f922234b3ad7919180` |
| `checkbox-fix-final-host.log` | `635ef9a3e78ed9e16f75a6fe75f15b87e6e74850287d89f45df779766b5c25db` |

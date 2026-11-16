# Two-route experimental map: initial human play

On macOS arm64, client `1.8.0.fe3adf3` directly opened the local
`reserve-all-3` generated and post-processed experimental sample in a fresh
profile. The copied VMAP SHA-256 is
`fc0a78024e32c3ace86f7829f02869dcd568940006bce1f9d0620c18bc027a43`.
This map contains test-only spells and skills. The isolated profile included
a local copy of `vcmi-test` with `keepDisabled` disabled, and separate saves;
the sample is not a supported distributable map.

The client observation reported a 108×108 toroidal map and red hero Korbac at
`(28,84,0)` with 1,560 movement points. Using the local diagnostic command
channel and game-window screenshots, the hero moved south, selected and
followed a path to an ore pile, raising the ore count from 20 to 26. The hero
then approached the adjacent ore pit; the running game's log recorded the
visit and the capture dialog promising two ore per day. The dialog was
sent an acknowledgment shortcut. A later screenshot showed the dialog still
open; clicking its confirmation button closed it. The last observed hero
position before the button click was
`(27,88,0)`, with 537 movement points.

The map, isolated profile, logs, observation stream and window captures are
under TB5 `runtime/seam-rmg-play-qa-2026-11-16/`. The screenshots are local
game assets and are not committed. These observations verify startup, normal
movement, resource pickup and one mine visit in the starting zone. They do
not verify a seam crossing, battle, opponent turn, a complete day, balance,
or terrain animation and seam appearance. Computer Use did not enumerate the
directly launched client; the window was captured by its macOS window ID and
input was delivered through the explicitly enabled diagnostic socket.

## Reached the southern edge

The same game was continued through day 3 using the diagnostic GUI channel.
The hero travelled around lakes and mountains from `(27,88,0)` to `(14,107,0)`
on the 108×108 map, reaching its southernmost coordinate with 219 movement
points. The daily ore total rose from 26 to 28 and then to 30. A subsequent
south shortcut was accepted for dispatch but left both the position and
movement points unchanged. Mountains were visible immediately across this
part of the seam; this observation alone does not determine the exact cause
of the rejected move. The hero did not cross the seam in this session.

The targeted window screenshot is `south-edge.png` in the local TB5 profile.
The viewport and minimap showed map content on both sides of the coordinate
boundary. This single still image does not verify continuous animation, the
quality of terrain joins or a guarded seam passage.

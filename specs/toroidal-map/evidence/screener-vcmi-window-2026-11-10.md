# Real VCMI observations and Screener window captures

On macOS arm64, the signed VCMI client built from `c2457bd` ran the Toroid
Seam Laboratory with the version 2 local observation trace enabled. The bridge
from VCMI revision `245b38a` started a separate Screener MCP process built from
Screener revision `d3b08b4`, selecting the game's window by bundle ID, window
ID and process ID. The existing connected, read-only MCP was not replaced.

The game was put in windowed mode for this check: in the preceding fullscreen
state the on-screen window enumeration did not list it in the current space.
After the check it was restored to fullscreen. Existing macOS screen capture
permission was used; requesting or denying permission was not tested here.

Two camera movements produced seven observation events, sequences 37–43, and
seven corresponding PNG keyframes. No extra keyframe was recorded during the
approximately 16-second gap between the two groups. The source trace reports
centers changing from `[83, 1136]` to `[223, 1136]`, then from `[223, 84]` to
`[223, 136]` on a 36-by-36 map with 32-pixel tiles. The saved timeline contains
the start, all seven named keyframes, and the stop event. One saved image was
visually inspected and shows the actual game window, hero and town.

The stored trace is local to TB5 at
`/Volumes/TB5/Developer/Builds/GitHub/Screener-mcp-window-recording-b01fcf1b5397bc1/runtime/vcmi-traces/4654E3F9-3123-4F08-BF75-8780D8B92B44.vtrace`.
The client source events are in the VCMI TB5 build tree at
`runtime/observation-player-c2457bd.jsonl`. Game assets and screenshots are not
included in this repository.

Each screenshot is requested after its associated observation; capture is not
atomic with the game state. The two processes' monotonic timestamps have not
been calibrated against each other. These sparse checkpoints establish the
integration for this window and session, not frame pacing, seamless animation,
all game views or behavior in every fullscreen/space configuration. The newer
version 3 visibility observations and later game fixes were not in this client.

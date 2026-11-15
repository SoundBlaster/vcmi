# Current graphical and headless AI games

On macOS arm64, the client, server and engine library were rebuilt from clean
revision `06ea341`. A separate local app was assembled from the previously
signed preview, with the new client, server and library. It retains the
existing preview bundle identifier, was signed with the authorized Apple
Development identity and passed `codesign --verify --deep --strict`. Client
and server reference the library inside the bundle. Other dependency paths
still refer to the local TB5 toolchain; this is not a portable release.

The graphical client was invoked directly with `--spectate`, `--testmap
Maps/toroid-demo-clean-2026-11-15.vmap`, and the spectator battle-skip options.
The screenshot observed through Computer Use showed the game map and version
`1.8.0.06ea341`; the screenshot was not retained. Both sides used Nullkiller2.
The server log records successful movement packets across both axes; for
example, red's hero moved from anchor `(1,0,0)` to `(36,0,0)` with result
`SUCCESS`, and a later move crossed from `(36,0,0)` to `(35,35,0)`. Anchor
positions are not the same representation as canonical visitable positions.
The log records blue's defeat and red's victory on day 9. The spectator client
then exited with code 0 and logged a normal server shutdown. The exit dialog
also emitted the previously documented GUI-thread wait warning; successful
exit does not resolve that warning. No manual save was made; VCMI may have
created an autosave directory.

The client log, standard output and observation trace are under
`runtime/gui-ai-06ea341-2026-11-15/` in the TB5 build root. Selected SHA-256:

```text
6b097fd6c0a004bfc7b7c85ae88c8ed202a41d84a1e3b067b1dfc8fca21f4a38  vcmiclient
25835dae9d890aecb8a74d0a4db6dcd1d3c117e5f8f23ac0c82b2b9159a25ff4  vcmiserver
89772b580d6135062b7ebdf5e002a2be4fe5fcfe01e06ad21d36eb4b111bc150  libvcmi.dylib
3915db73a1a6d4c65ccc88cf0d1cd6e6f40e32802e4db668d8cba471abf32cb3  VCMI_Client_log.txt
536e5e3153b6fb2256006507b24ad4113ec49744df9a62bf921c5ae226a101e1  observation.jsonl
9a945a811529805ac53c28352027e01129446f434847497b0512d770600e1c0a  toroid-demo-clean-2026-11-15.vmap
```

A separate headless run of the same map with two Nullkiller2 players reached
day 224 without game over and was stopped with SIGINT (exit 130). Its log
also records successful wrapped movement and later repeated turns where no
worthwhile task was found. This minimal laboratory map has two towns, two
starting heroes and three resources; the observation does not establish the
cause of the long game or general AI behavior. Its logs are under
`runtime/headless-ai-06ea341-2026-11-15/` on TB5.

Both sessions used an in-process local server and no external peer. Although
`--serverport 3033` was passed, the log reported configured port 3030; the
local single-player server did not listen for incoming connections. The
option's behavior for an external listener requires separate investigation.
Neither session proves remote multiplayer, smooth scrolling, animation
continuity, player-only fog confidentiality, or broad AI support. The map is
a sparse functional fixture, not a balanced playtest map.

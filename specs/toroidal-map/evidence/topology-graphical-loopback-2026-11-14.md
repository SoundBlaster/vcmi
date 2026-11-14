# Two graphical clients on loopback

On macOS arm64, two separately named development-signed app bundles used the
client, server and library built from `30a5dc1`. Both signed bundles passed
`codesign --verify --deep --strict`. Each client was started directly with its
own private command socket, observation file and temporary local log; both
used the same VCMI user-data directory. The host used an in-process server.
The guest joined it at `127.0.0.1:3030`, and the server assigned red and blue
to different client connections. The selected map was the existing 36×36
`Toroid Seam Laboratory` fixture.

Both clients opened the game. Their initial observation records reported
`toroidal: true`, the same map dimensions, and distinct player identities.
Red's selected hero started at `(0,0,0)` with 1,560 movement points and
20,000 gold. A northwest command visited the gold at `(35,35,0)`, producing
21,000 gold and 1,419 movement points while the hero remained at `(0,0,0)`.
Another northwest command moved the hero to `(35,35,0)` with 1,278 points.
The guest log recorded both corresponding `TryMoveHero` packets.

After red ended its turn, both client logs reported that blue's turn started.
The server reported a successful `Autosave-111` before the turn transfer.
Blue then moved east from `(18,18,0)` to `(19,18,0)`, from 1,630 to 1,530
movement points; the host log recorded the move packet. The autosave predates
this blue move. Player observations are filtered to the selected player's
state, so these records do not independently compare each client's full game
state or establish fog confidentiality.

The guest logged several `accessing unavailable frame 0:2 in CAnimation`
errors when joining the lobby; the host logged one. The game nevertheless
started, but the missing animation and its cause were not investigated here.
The server listened on all IPv4 interfaces even though the guest connected
over loopback. This local run does not establish remote-host behavior,
continuity of animation, or successful reload of the autosave.

A point-in-time copy of the local evidence is under TB5 build root
`runtime/gui-multiplayer-30a5dc1.iMinul`:

```text
d5a0b8b280d73f987628d68a701c02a7c2af38d2706181ab8dfbba4d29cc8dca  host-log.txt
bf2f0eb4eddf1953a6e1a96b369e2463e5ecefc55d56ca96cfb19c375609bbc2  guest-log.txt
a5c51271f8616da483e9e41135b6ae3b9c4e116dc990566e88e9a31d0c305418  host-observation.jsonl
c17ce3b9069d0f104408898f95a58ba332ddd0d92cc46aad50d0c3317bc8a7ae  guest-observation.jsonl
85042aa9362192df75b9b346644c3c3d8dc29ca6d96075b1ee10e6633be6a742  autosave-111.vsgm1
```

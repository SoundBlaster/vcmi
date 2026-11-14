# Nullkiller turns and battle across map seams

On macOS arm64, the signed `VCMI Toroid Rect Size Trial.app` was launched
directly with a private diagnostic socket and log. Its client identified
itself as `1.8.0.ebeb910`; it was built from that revision plus the
rectangular scenario-list fix, subsequently committed as `e6c8fff`. The
bundle's client and engine library hashes are recorded in the
[rectangular-map record](topology-rectangular-vmap-2026-11-15.md).

A new single-player game started on the untagged prototype `Toroid Seam
Laboratory`, SHA-256
`3fbc0a2cbbdfa22068e7e6a099a7b51e803d39b5fc0d0b3fd95155cc6eeb232d`.
The client reported blue was controlled by Nullkiller2. Red ended its first
turn without moving. Blue took a real AI turn: among other actions, Ryland
requested visitable tile `(32,0,0)` from `(31,35,0)`. The server logged the
corresponding anchor movement `(32,35,0) -> (33,0,0)`, applied the movement
packet, and reported the request successfully applied. Blue ended its turn
and the server gave the next turn to red.

Red ended its second turn. Blue's Ryland requested an attack on the red hero
across the east-west seam; the server logged anchor coordinates
`(36,1,0) -> (1,0,0)`, a hero visit and `BattleStart`. The actual battle was
played with red's auto-combat enabled. The result screen showed Ryland winning;
after its confirmation, the server applied the battle results and removed an
object, and the client processed the removal. Nullkiller ended its turn and
the server gave red the next turn. The screen then showed red's town with no
hero selected. The session was closed without a manual save; the game can
create autosaves when ending a turn.

The client log, which also includes the embedded server messages, and the
player-filtered observation stream are in
`runtime/gui-nullkiller-seam-battle-2026-11-15/` under the TB5 build root.
Their SHA-256 hashes are respectively
`049df105423a53c2f6ff2dd331ed6d442f7753560ef4cc3cb7ff15bdfcbc716f` and
`08a0a283f5f99ae979ea584a092ba3628a28aa491fad28cbcb4b51d45a1d5514`.
The battle and subsequent screen were observed but screenshots were not
retained.

This is one session on a sparse fixture using the built-in client/server
connection. It is not a remote multiplayer test, a balanced game, a proof of
all AI choices or a visual assessment of seamless movement animation.

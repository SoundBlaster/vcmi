# Current-build boat corner observation

On macOS arm64, the signed development client displaying version
`1.8.0.c4aa1ed` loaded the existing 36×36 **Toroid Open Water Corner**
saved game. Its observation record reported `toroidal: true`. This run used
the user's Heroes III resources and the configured VCMI profile on TB5.

The selected hero initially had zero movement points. Ending the turn started
day 2 and restored 1,500 points. The player used numeric-keypad movement to
sail from `(31,30,0)` to `(35,35,0)`, then diagonally across both seams to
`(0,0,0)` and back to `(35,35,0)`. The observation records report 836, 695,
and 554 movement points at those three positions respectively. The GUI showed
the boat near the center of the view and the minimap marker in the matching
opposite corners after the two crossings.

The camera center samples around the forward crossing were `(1136,1136)`,
`(1144,1144)`, `(14,14)`, `(16,16)`; for the reverse crossing the samples
were `(16,16)`, `(9,9)`, `(1140,1140)`, `(1136,1136)`. The map period is
1,152 pixels in each axis. These samples are consistent with short wrapped
camera travel, not an observed full-map pan. Samples and screenshots are
sparse, so they cannot establish frame pacing or uninterrupted animation. This
run does not cover embarkation, shores, all corners, multiplayer or AI.

The 29-record observation capture is retained on TB5 as
`runtime/observation-player-c4aa1ed-water-corner-2026-11-10.jsonl`; its SHA-256
is `e1e0686015d7ea65e78e2d87d42324d04fe6b1dbff0b9398cfc963f8684408ac`.
The screenshots were observed through Computer Use during this session; they
are not separately archived with this evidence.

# Crossroads: multiple AI turns and player defeat

On macOS arm64, the signed preview built from `9f20a0b` was launched directly
with a private diagnostic socket, log and observation stream. `fde0c16` was
the branch head, with documentation-only changes after the build. A separate
copy of the versioned `Toroid Crossroads` map was named
`Toroid QA Multiday fde0c16` in its Russian header. Its SHA-256 was
`480c504ef3c7dd2e155ff9fb091d88d290cb842c85943acd0746fbd44fd7b02f`.
The loaded map was 36 × 36 and toroidal. The copy and its scenario save
directory were distinct from the existing test save.

The red player used the diagnostic channel to end each turn without moving
or developing the starting position. Blue was controlled by Nullkiller2.
Over six blue turns the server logged 149 movement attempts initiated by its
heroes Valeska and Tyris. Five blue turns ended and returned control to red;
on the sixth, the match ended during blue's turn. The movements included
crossings of both horizontal and vertical seams. For example, Tyris crossed
from raw hero anchor `(36,25,0)` to `(1,24,0)`, while Valeska crossed from
`(22,35,0)` to `(21,0,0)`; the former uses hero anchor coordinates, which
are offset from their visitable tiles. The log also records building
construction, a neutral battle and blue building in red's former town.

On day six Tyris attacked the remaining red hero at `(0,0,0)`. The client
waited for the human player's battle actions; built-in auto-combat was enabled
through the battle UI. Blue won the played battle, the server emitted
`PlayerEndsGame` and the client reported red's defeat. Dismissing the result
and defeat dialogs caused the client to stop the game and display the loss
sequence. The test process was force-stopped afterwards; normal application
exit was not established. No transition to a second week occurred.

The log and observation stream are in
`runtime/gui-multiday-fde0c16-2026-11-15/` under the TB5 build root, with
SHA-256 `154dcd65f0e2c698eb69c25257415ea53da62142fca8b4c334b1e36f9ad58c64`
and `7365bd16f62de1cdc189ea6d190b41eb4500c9515a41399493adf68304570116`,
respectively. This run shows one short match against an inactive player, not
balanced play, comprehensive AI support, the accuracy of every movement
attempt, remote multiplayer, a week transition or visual smoothness.

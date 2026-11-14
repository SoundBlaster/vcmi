# Guarded mine visit across a seam

`HeroMovementTest.GuardedMineAcrossSeamPreservesVisitThroughBattle` moves a
hero north from `(10,0,0)` to a guarded neutral mine at `(10,35,0)` on a
36×36 toroidal map. The server opens the mine's actual blocking dialog. A
refusal leaves the mine neutral and closes the visit; revisiting the same tile
and accepting opens a battle against that mine while the visit remains active.
After a server-side forced victory and acceptance of the result dialog, the
mine belongs to the hero's player and the visit has finished.

On macOS arm64, this test and the existing unguarded capture and same-tile
revisit tests passed. The battle outcome was forced by the server test helper;
this does not establish graphical battle play, a remote client, other seam
directions, underground interaction, or defeat and retreat behavior.

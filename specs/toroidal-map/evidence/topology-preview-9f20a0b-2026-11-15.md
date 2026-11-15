# Corner crossing and save/reload on the current preview

On macOS arm64, the client, server and engine library were built from clean
revision `9f20a0b` into the separate TB5 build tree. A local preview bundle was
prepared from these binaries and an earlier bundle using
`prepare_macos_preview.py`; its Apple Development signature passed deep strict
verification. The client was started directly with a private diagnostic socket,
log and observation stream; this run does not establish a Finder launch.

The test used an isolated copy of the versioned `Toroid Crossroads` map, named
`Toroid QA 9f20a0b` in its Russian header, to keep its saves separate. The
original VMAP SHA-256 was
`dc22f9087595505366d7e6c6ff585df4e9b265436a18a2c7d6d539493848735f`;
the renamed copy was
`a37f3c33cf22a559930820eb4e1630f1d88d4e626ce7c64d51446bda86d55269`.
The loaded map reported 36 × 36 tiles and toroidal topology.

Using the diagnostic command channel through the graphical client:

1. Two northwest commands collected 1,000 gold and moved the red hero from
   `(0,0,0)` to `(35,35,0)`, crossing both seams. Movement fell from 1,560 to
   1,278 and gold increased from 20,000 to 21,000.
2. The in-game save dialog wrote a uniquely named save in the test scenario's
   directory. A southeast command then moved the hero back to `(0,0,0)` with
   1,137 movement points.
3. The in-game load dialog loaded that save. The client observation again
   reported toroidal topology, hero position `(35,35,0)`, 1,278 movement
   points, 10 mana and 21,000 gold. The server log reports successful loading.
   The client exited through the in-game confirmation.

The log and observation stream are in
`runtime/gui-9f20a0b-save-check-retry-2026-11-15/` under the TB5 build root.
This is one surface-level route and one local save/reload, not a complete
state comparison, a full playthrough, a remote multiplayer test or a visual
continuity assessment. The user will assess smooth scrolling separately.

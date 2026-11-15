# Current preview: AI, save and reload

On macOS arm64, the local `runtime/VCMI Toroid Preview eeb3540.app` under the
TB5 build root was assembled from the existing client, server and library
build outputs after `4791c1c`. Subsequent changes through `eeb3540` affected
tests and documentation, not runtime source. The binary reports version
`1.8.0.675f11b`, because its version stamp predates the last runtime source
commit. The client and server explicitly reference the library inside this
bundle; `lsof` confirmed the client actually loaded that copy. Other build-time
library search paths still point to the local TB5 toolchain, so the bundle is
a local preview rather than a portable distribution. It was signed with the
authorized Apple Development identity and passed deep strict verification.
No Finder/Launch Services startup or notarization was tested.

The directly invoked client opened `Toroid Seam Laboratory`, the earlier
untagged 36×36 prototype (SHA-256
`3fbc0a2cbbdfa22068e7e6a099a7b51e803d39b5fc0d0b3fd95155cc6eeb232d`).
The diagnostic channel's two northwest commands collected 1,000 gold and
moved the red hero from `(0,0,0)` to `(35,35,0)`, leaving 1,278 movement
points. Red ended two turns. The client identified blue as Nullkiller2. On
the first blue turn the server applied an anchor move `(18,35,0)` to
`(17,0,0)`; on the second it applied `(15,0,0)` to `(14,35,0)` and a
different hero's `(1,22,0)` to `(36,23,0)`. These are anchor coordinates;
both AI turns ended and the server returned control to red. This was an
embedded local server, not a remote multiplayer game.

The game created `Autosave-112.vsgm1` (SHA-256
`860b3bf7781bd8bb8e3d85a2b653bec0b5586933e86bd888fa89531de6e1eb04`)
before the second blue turn. Loading it through the GUI restored the observed
36×36 toroidal map, red hero at `(35,35,0)`, 1,560 movement points and
21,500 gold. The observation records immediately before saving and after
loading matched for camera center, visible fog runs and the player-state
fields recorded by the diagnostic channel. After reload a southeast command
returned the hero to `(0,0,0)` with 1,419 movement points. This comparison
covers only that player's observed viewport and fields, not all game state.
The client closed normally and no listener remained on port 3030.

During map loading the older prototype referenced two unavailable test-mod
spells. The separate [clean demo record](demo-test-mod-clean-2026-11-15.md)
describes a corrected fixture. The run also logged `Cannot wait for dialogs
in gui thread (deadlock risk)!` when presenting end-turn, load and exit
confirmations. Source inspection shows `showYesNoDialog` calls
`waitWhileDialog`, which logs and returns instead of blocking on the GUI
thread. Those confirmations were displayed and accepted in this session; this
does not establish correct ordering for already-active or nested dialogs.

The client log and player-filtered observation trace are in
`runtime/gui-preview-eeb3540-2026-11-15/` on TB5. Screenshots were observed
but not retained. Neither this run nor the selected observations establish
continuous animation, all AI choices, full fog equivalence, or remote play.

SHA-256:

```text
3f5f180e89ae436271d01905340535873f25a97b493b9758bb326f060ad4048f  vcmiclient
ec1a71239bf8a63ff50afeb706dc4f6e53adbb47b007e17fe0c6144849183fc0  vcmiserver
25e1580fc58a0e7dad44046c57c3860744f9fb6326d2948d59cba1ba4331cab9  libvcmi.dylib
9ecc01bc71c899ac8ae3e49347c5697c04ea4e0ed0437e84d850482ceb62e291  VCMI_Client_log.txt
e718d1719b70f00fb72d969495eeda62ab8fd4a06e840207226db06fd277006c  observations.jsonl
```

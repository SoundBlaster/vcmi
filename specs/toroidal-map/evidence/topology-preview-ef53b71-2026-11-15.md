# Current preview: AI battle and corner crossing

On macOS arm64, `runtime/VCMI Toroid AI Check ef53b71.app` on TB5 was copied
from the previous diagnostic bundle. Its client and `libvcmi.dylib` were
replaced by the build made from `062c82d` plus the changes committed as
`cc64f09`; `ef53b71` adds only documentation. The client was set to load the
library inside the bundle. The app was signed with the authorized Apple
Development identity and passed deep strict signature verification. Its window
reports the earlier generated version stamp `1.8.0.062c82d`. This is a local
preview whose other library paths can use the TB5 build toolchain; Finder
launch and portability were not tested.

The directly launched client opened `Toroid Seam Demo`, 36×36, using the
installed Complete data. The player observation channel reported a toroidal
map. Two northwest commands collected 1,000 gold and moved the player's hero
from `(0,0,0)` to `(35,35,0)`, leaving 1,278 movement points. On ending the
player turn, the server gave control to blue, whose adventure AI was
Nullkiller2. Blue attacked the player's hero; the actual battle screen was
played using automatic combat, and the result displayed blue's victory.
After acknowledging it, the AI continued its turn. The AI log reports Crag
Hack moving from `(35,35,0)` to `(0,0,0)`, and the server log records the
corresponding move using anchor positions `(36,35,0)` to `(1,0,0)`. Blue
ended its turn and the server returned control to red on day two. The client
and its local server exited normally, and port 3030 had no listener afterward.
The game created an autosave; no manual save was made.

This is a single local game and one battle; it does not reproduce an empty
cluster during AI play, establish general AI behavior or multiplayer, verify
the new fractional-scale radar border visually, or measure smooth animation.
Screenshots were observed during the session and not retained. The client
log and player-filtered observation trace were copied to
`runtime/gui-preview-ef53b71-2026-11-15/` under the TB5 build root.

SHA-256:

```text
9a945a811529805ac53c28352027e01129446f434847497b0512d770600e1c0a  toroid-demo-clean-2026-11-15.vmap
db8d83668e8aa7b62b84502a0c4d611bbbd90e704abfb5a239f084734bc5691f  vcmiclient
ea492ea8643f2b71d4bb2640ddd27605f63babd3a03cab3b95dd4a2529c66974  libvcmi.dylib
c90d53cbb3d182f543a58d16cd0fbcbb0aa9962f340054aae7f8ef6279f267f0  VCMI_Client_log.txt
ca9a86c3968d61cbaa4f9bfdd69dae37e8528ea208fa3b9b2977fe5d824a2935  observations.jsonl
```

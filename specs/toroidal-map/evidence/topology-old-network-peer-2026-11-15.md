# Original VCMI client in a toroidal lobby

A real graphical client from the original `30e9207276f137658da5132705c05644bfe33aee`
baseline joined the experimental host's TCP lobby at `127.0.0.1:3030` on
macOS arm64. The host had selected the 36×36 `Toroid Roads Demo` VMAP. The
server assigned one player to each connection. The old client entered the
lobby and displayed the selected map's name. When the host attempted to
start, the server initialized the candidate game state, rejected the start
because of the incompatible client, and remained in the lobby. No adventure
game began. Both clients exited normally.

The first run used the host client built from `68e6b3b` plus the previously
documented object-template fix. Its warning misleadingly said the server
version was incompatible with the local client. The server log identified
the actual condition: an incompatible client in a toroidal game.

The server now resolves a specific localized message before sending it as raw
text, since the old client does not have the new translation key. A repeat
using a host built from `46f05d2` plus this message change and the same
unmodified old client produced the intended English message in both windows:
one player lacks toroidal support and all players need compatible versions.
The server again rejected the start. English and Russian translations were
added, but the Russian message was not visually tested; all peers receive the
server-resolved language, not their individual client languages.

The baseline client SHA-256 was
`1edf3d0eaad617e04e4c45e93fcdfa61fdab4f7f3a57d4bddfa4dc7bfb5b0247`.
The second signed host client SHA-256 was
`d99acdb408d3b44e8b0c1294cf3a97009161cf9555979f4e7ae7b44f317257e7`.
Both runs used the clean demo map with SHA-256
`b451f983719e345fed98cc13f6ac5bd5472a6b3b943a4bfd60c68c291213bc74`.
Logs from both clients and runs are under `runtime/gui-old-peer-2026-11-15/`
in the TB5 build root. The baseline client logged unavailable animation frames
in the lobby. The server listened on all IPv4 interfaces, although the guest
used loopback. Port 3030 was not listening after exit. Screenshots were
observed, not retained.

The `TcpLobbyStartsToroidalGameForTwoPeers` test also verifies that modern and
legacy-layout peers receive the resolved warning, the server remains in the
lobby, and two compatible peers can start after the incompatible peer leaves.
That focused test and the complete macOS arm64 suite passed: 1,726 tests and
three disabled. Reports are `runtime/old-peer-message-2026-11-15.xml` and
`runtime/full-suite-old-peer-message-2026-11-15.{log,xml}` on TB5. The build
reported duplicate-library and missing Conan search-path warnings. This test
does not cover remote hosts or other baseline versions.

## Original host with a new client on a bounded map

The reverse arrangement was tested with the same original client, hosting its
original server, and the `46f05d2`-based experimental client described above
as the guest. Both graphical clients ran on macOS arm64 and connected over
`127.0.0.1`. A separate copy of the clean 36×36 demo was converted into a
bounded version-3 VMAP by removing its topology field and experimental format
marker and giving it a distinct name. It had two human slots; the map copy's
SHA-256 was `c5bdd57645d7329f23225cda4491f9005ebffaa9b352e3b0da1d3d046aeb3f1c`.
The original demo's hash remained unchanged.

Both clients entered the game. Red moved east under the original client, then
ended its turn. Blue moved east under the experimental client, whose player
observation changed from tile `(18,18,0)` and 1,630 movement points to
`(19,18,0)` and 1,530 points. The original server logged the corresponding
successful movement and broadcast its movement packet to both clients. Blue
ended its turn and the original client received the red turn on day two.
Red returned to the western edge; a further west key press caused no new
movement request in the host log. The new client's observation reported
`toroidal: false`. The game created an autosave in a new scenario directory;
no manual save or reload was performed. Both client processes exited with
code 0 and port 3030 was no longer listening.

The server's log uses the hero object's anchor, which is one column east of
the player-observation tile in this case; the coordinate values are not the
same representation. Logs and the guest observation are in
`runtime/gui-legacy-host-2026-11-15/` on the TB5 build root. An earlier
eight-player attempt reached initialization but the original client aborted
because its test build lacked the configured Nullkiller2 AI. The two-human
run avoided that test-build limitation. The original build also displayed
garbled Russian text and logged missing lobby animation frames.

This is one bounded map and one local mixed-version session. It does not
establish full state or fog equivalence, mixed-version save reload, remote
network behavior, all original versions, or compatibility with toroidal maps.

## Reading the original server's bounded autosave

The original host's `00 Bounded Two Humans Test` autosave from the session
above was inspected with the current branch on macOS arm64. Its SHA-256 was
`879f68ef85f95a08a9b17238daed0d73058fcf65dc2e7c42bb6cd8564c6b6d4d`
both before and after the test. `ToroidalDemoVmapTest.DecodesOriginalBoundedSaveAndMovesHero`
from commit `bbe64c4` decodes the header, game state and server state directly,
checks that the 36×36 map remains bounded, then uses the in-process server
handler to move the blue hero from `(19,18,0)` to `(20,18,0)` and verifies
that movement points decrease. The test passed; its XML is local to TB5 at
`runtime/original-bounded-save-2026-11-15.xml`. The executable contained the
test change on top of `c92782c`; linking reported duplicate-library and
missing Conan search-path warnings.

This bypasses the normal loader's mod compatibility gate and the multiplayer
lobby; it does not establish graphical loading or an actual continued turn.
The test uses a specific local fixture and is skipped unless
`VCMI_ORIGINAL_BOUNDED_SAVE` is supplied. It does not modify the input save.

The follow-up `ToroidalDemoVmapTest.LoadsOriginalBoundedSaveThroughServer`
in `1d0da07` loads the same original save via `CGameHandler::load` with the
isolated user-data profile used for the mixed-version game. It supplies the
file through a temporary read-only resource loader, verifies the selected
path and exercises the normal saved-mod compatibility check. The loaded map
remains bounded; a new in-process handler acting on the loaded state moves the
blue hero from `(19,18,0)` to `(20,18,0)` and deducts movement points. On
macOS arm64 the test passed with the test source on top of `de19cf0`; its XML
is local under TB5 at `runtime/original-bounded-normal-load-2026-11-15.xml`.
The input SHA-256 remained unchanged. The test does not drive a graphical
client, network lobby or a complete resumed turn; it uses a separate test
handler for the move after the production load path returns.

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
`d99acdb408d3b44e8b0c1294cf3a97009161cf9555979f4e7aeec98def4bc67e7`.
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
does not cover an old server hosting a new client, bounded mixed-version
gameplay, remote hosts or other baseline versions.

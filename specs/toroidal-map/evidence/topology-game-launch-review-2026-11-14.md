# Game launch and open-water corner crossing after review

On macOS arm64, client and server were built from clean revision `053a830` on
TB5. A separate bundle was signed with the user's authorized Apple Development
identity and passed `codesign --verify --deep --strict`. Its client window
reported `1.8.0.053a830`. The client loaded the configured Heroes III resources,
reached the main menu, and loaded an existing **Toroid Open Water Corner**
save. The observation layer reported a 36×36 toroidal map.

Through the real graphical client, the selected hero ended day 1 at
`(31,30,0)`, regained 1,500 movement points on day 2, and sailed to
`(35,35,0)` with 836 points. A southeast step crossed both seams to `(0,0,0)`
with 695 points; a northwest step returned to `(35,35,0)` with 554. The
minimap marker moved between the corresponding corners. GUI screens were
observed through Computer Use; the sampled state record is retained as
`runtime/observation-player-053a830-corner-capture-2026-11-14.jsonl` on TB5.
Its SHA-256 is `760500f571bab33154b443e599f3f040ef880f785576ed65c58a964e250b4a01`.
The screenshots were not separately archived. This confirms the observed
startup, saved-game load, turn transition and two boat steps; it does not
prove continuous frame pacing, all edges, all movement rules or multiplayer.

Before packaging, the full `vcmitest` suite passed 1,495 tests from 226 suites
with no skips and four disabled, including the untagged prototype save and map
and the Complete `Peacemaker.h3m`. After removing an unused include, the same
server and test targets rebuilt and 12 focused spell and AI checks passed.
The full-suite log is `runtime/full-suite-spell-fog-review-final-2026-11-14.log`
on TB5. This is local verification, not hosted CI or full feature acceptance.

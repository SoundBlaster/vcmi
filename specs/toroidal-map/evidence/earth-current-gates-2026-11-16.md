# Earth demo: converted gate pairs

The bounded two-level converted source has four subterranean gates. The
assembler's source-pairing calculation identifies `subterraneanGate_5430`
with `subterraneanGate_10528`, and `subterraneanGate_5431` with
`subterraneanGate_10533`. In the current 252×576 single-level demo these
same named objects use isolated two-way-monolith channels:

| Source pair | Draft subtype | Draft object anchors |
| --- | --- | --- |
| 5430 / 10528 | `monolith2` | `(3,388,0)` / `(250,388,0)` |
| 5431 / 10533 | `monolith4` | `(3,416,0)` / `(250,416,0)` |

These are object anchors, not necessarily visitable tiles. The four objects
retain their source gate appearance and footprint; there are no remaining
objects of the subterranean-gate type in the draft. Replacing the gates is
an intentional gameplay change: a monolith does not preserve every gate
interaction, including the original friendly-hero exchange at an occupied
exit. This mapping does not claim that all original regional routes or game
balance are unchanged.

On macOS arm64, `MapFormat.InitializesExternalToroidalMapAndChecksPortalPairs`
initialized the current local draft, SHA-256
`67640eefaa91e9efcf04315bef66197d834e662652bd25bd10f234391c43e5a8`.
With `VCMI_INSPECT_APPROACH_PORTALS` and `VCMI_INSPECT_REQUIRE_ALL_GUARDS`,
it checked the two separate two-entrance/two-exit channels and staged a hero
on a neighboring tile for each of the four approaches. Normal server movement
and portal activity reached each paired exit; four guard victories and two
level-up choices were forced by the test. It passed using the `0aba38b` test
build plus the unrelated detached-callback test change; the XML is
`runtime/earth-current-gate-approaches-2026-11-16.xml` under the TB5 build
root. This does not prove a route from a starting town to each gate, ordinary
battle difficulty, GUI presentation, remote multiplayer or whole-map
connectivity. The original map and game assets remain local.

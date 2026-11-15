# Earth draft: graphical corner crossings

On macOS arm64, the SDL3/Metal client built from `32936c5` with shortcuts
from `17c6a69` started a human-controlled red game from the normal scenario
list, on a staged 252×576 toroidal Earth copy named `Earth Corners QA`.
The source was the Earth draft SHA-256
`58e7b7e6bf94c117d350028757c9cf1c7160bf3107fe435cf003306d3dbd74a6`.
The fixture changes the scenario name, moves red's starting hero anchor from
`(165,353,0)` to `(1,0,0)`, and sets the four corner terrain tiles to snow.
Its SHA-256 is `02b0016fe30a1afa813e8d9d7a9c39ae9916cc5a0c006ef3df1d78413d6d25e6`.

| Command | Visitable tile after move | Movement points |
| --- | --- | --- |
| Start | `(0,0,0)` | 1,560 |
| Northwest | `(251,575,0)` | 1,348 |
| Southeast | `(0,0,0)` | 1,136 |
| West | `(251,0,0)` | 986 |
| Northeast | `(0,575,0)` | 774 |
| Southwest | `(251,0,0)` | 562 |

The four diagonal moves each crossed both axes, including both directions of
both corner pairs. The prior [vertical fixture](earth-gui-vertical-save-2026-11-15.md)
had water at `(251,575,0)`, the target of its unsuccessful northwest attempt;
the on-foot hero could not use that water as a normal land destination. That
fixture has not been shown to contain a corner topology defect.

The client and local server exited normally; client exit code was 0. Evidence
is local under TB5 `runtime/earth-corners-gui-profile/`. Observation SHA-256:
`14f2fecaa25e1506e2e4401659847a866f4c1b21ba9351df37ca0ae349f71a88`;
client log SHA-256:
`0e607a80208c46e5ed99eea3b165dc430d36fb11e70c7a48ed78d401898b405e`.
The map and game assets are not committed. This checks a prepared land fixture,
not original Earth corner passability, all terrain interactions or animation
continuity. It does not demonstrate multiplayer or complete playability.

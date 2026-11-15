# Earth draft: exploratory road across the horizontal seam

`MapFormat.PreparesExternalToroidalSeamRoad` opens an explicitly supplied
VMAP, searches for the longest symmetric clear land segment up to three tiles
on each side of the horizontal seam, and uses the map editor's road operation
to draw dirt road. It excludes blocked, visitable, existing-road and river
tiles, then saves and reloads the map and compares the selected road types,
directions and flags. The output must not already exist. This opt-in test does
not distribute the map or its assets.

On macOS arm64, the binary built from `c5ade84` plus the test committed as
`89dfd95` passed with the isolated Earth mod profile and the Earth draft
SHA-256 `58e7b7e6bf94c117d350028757c9cf1c7160bf3107fe435cf003306d3dbd74a6`.
The first qualifying site was row 425, with only two qualifying cells:
`(251,425,0)` and `(0,425,0)`. The final saved local test map SHA-256 is
`a8897652c82f1a44ff1d90a620420b5e288ab7b2a123978e4b42d4831968973c`.
The local files are under TB5 `runtime/earth-map-research-2026-11-15/`.

An earlier equivalent run produced local map SHA-256
`2b95e60b6e2743ed53801b4ce6812e1fdd956df551ad6e6e9abed329f553768f`.
For a graphical test, a copy of that output was renamed and its red hero was
staged at `(0,425,0)`; its SHA-256 is
`f3476ee13e49b67581402e8412b865bb3375ab6fdbe655f99e6b712da4d13df0`.
The SDL3/Metal client build `32936c5` with diagnostics from `17c6a69`
started that copy as a human player. West and east crossings each cost 75
movement points: `1700 → 1625 → 1550`. A subsequent north step cost 150.
The client and local server closed normally, client exit code 0.

The static screenshot `runtime/earth-road-gui-profile/seam-clear.png` has
SHA-256 `cc509d67062b6620c7a8b2a6131b5cf04c8b1e32b5349a6a28708981d3bff6f8`;
the observation stream SHA-256 is
`ab8839a6e713017d44104edf9480077bb8ba37e50f71504cfa86ad11b6bff34f`.
The added two-cell road looks like an isolated short fragment in that view;
it is not connected to the existing network and is not yet a convincing road
demo. Its placement and appearance need further work. This single frame does
not prove scrolling or animation continuity, and the test does not validate
the entire saved map, routes to the fragment, other seams or multiplayer.

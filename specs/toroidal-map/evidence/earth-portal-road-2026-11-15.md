# Earth draft: experimental road to a water monolith

`MapFormat.PreparesEarthPortalRoadFixture` (test committed as `b4b5981`)
creates an opt-in local variant of the Earth draft. It removes two specifically
identified non-visitable decorations, `yuccaTrees_7568` and `mountain_7569`,
and asks the editor to draw ten dirt-road cells between the existing road at
`(2,425,0)`, the horizontal seam and the southern approach to
`monolithTwoWay_5267` at `(250,429,0)`. The test checks the removed objects,
selected tiles, portal approach direction, and saved/reloaded road types.
The original map is not changed. On macOS arm64 the test passed using the
isolated HotA profile and a build based on `2978d52` with the test in the
worktree. Its output, `earth-portal-road-qa.vmap`, is local under TB5
`runtime/earth-map-research-2026-11-15/draft-stacked/`, SHA-256
`182c1fff77286356bc2b139eb18f68bc1f1f403e8636622d68a61787edae368f`.

In a first graphical run with an ordinary staged starting hero, the route
reached `(0,428,0)` but triggered a guard battle. The game's automatic battle
ended in defeat. A second local graphical fixture, SHA-256
`b013f26fe3e221c169b54a4d46acd8a01fd88c6584a0dd506d1d4f688624e481`, staged
the same hero with 500 archers; this is a test-only advantage, not demo balance.
The threatened neutral stack fled when allowed to do so. The hero continued
along the road via `(0,429,0)`, `(0,430,0)`, `(251,430,0)` and `(250,430,0)`.
Those four cardinal steps each cost 75 movement points.

From `(250,430,0)` the hero entered the monolith. The server attempted a
teleport from anchor `(251,429,0)` to `(170,285,0)` and rejected it with
`Cannot move hero, destination tile is on water!`. The hero remained at the
entry's visitable cell `(250,429,0)`. Both graphical client runs shut down
with exit code 0; this does not verify successful portal traversal.

Read-only inspection of serialized map data found four monoliths with subtype
`hotaPortalWaterBlue`: the visited entrance is on sand, while the other three
visitable cells are on water. The attempted exit corresponds to one of those
water cells. This mismatch is a concrete demo-map issue to investigate; the
evidence does not establish whether it originates in the source map, the
conversion or a separate portal rule. The road drawing also produces a dense
ladder-like pattern next to the original road in a static view and needs
design work before use as a visual demo.

The strong-fixture observation and client log are local under TB5
`runtime/earth-portal-strong-gui-profile/`, SHA-256 respectively
`1f4d316f10c8d920cbac757db0f3e72807fe7c416abd322a969cb49d34bbd04a` and
`3463b4d6bee196197668cf18fbedf830ba0d870a2aa66d83000073248374873f`.
The isolated fixtures do not establish map balance, complete scenario
compatibility, normal unassisted play or animation continuity.

## Source comparison and water-portal repair candidate

The converted two-level source places all four `hotaPortalWaterBlue` visitable
cells on water. In particular, `monolithTwoWay_5267` is at visitable cell
`(250,141,0)` on water. The initial stacked draft retained it on water at
`(250,393,0)`; the east-Africa rejoin changed that cell to sand. The later
polar transformation shifted the mismatched entry to `(250,429,0)`. Thus the
land/water mismatch was introduced by the draft adaptation, rather than being
present in this converted source.

`MapFormat.PreparesEarthWaterPortalRepair`, committed as `eb1108f`, constructs
a separate local map from `earth-reserved-guard-diagnostic.vmap`. It moves
only that monolith's anchor from `(251,429,0)` to `(240,429,0)`, checking that
its three active destination cells and a southern approach are unobstructed
water. It saves and reloads the map and checks the visitable position and
approach direction. The resulting local map SHA-256 is
`53f6a9f039fd1370b33956bb5702d3ccaa804d1ff3ebff0d7479a101ed69be83`.

An opt-in in-process server check in the same commit initialized that map,
staged a hero at `(241,430,0)`, created a boat at `(240,430,0)` and embarked.
After resetting movement points to simulate a later turn, the hero sailed to
`(239,430,0)`, entered the repaired monolith, selected a *different* exit and
arrived at water tile `(168,290,0)` in the same boat. The test also checked
the existing two isolated converted gate pairs. Both the map generation and
this server check passed on macOS arm64; the latter's TB5 XML at
`runtime/earth-water-portal-game-qa.xml` has SHA-256
`d77b150b5650f035d4ca82a8b20dd2380424113e83aca8f67f2eb818ac1bafad`.

The repair is a candidate, not yet integrated into the assembler or ordinary
demo profile. This checks one boat approach and one exit on the server; it
does not establish all other portal exits, GUI animation, normal route to the
boat, broader collision effects or source-equivalent balance. The failed
land-road-to-water-portal experiment above remains a separate local fixture.

## Reproducible assembly and local demo profile

Commit `b4930b4` adds explicit `--relocate-water-monolith INSTANCE X Y`
support to the diagnostic assembler. It checks the source water entrance,
object type and subtype, destination water footprint and occupancy, and at
least one permitted clear water approach. These are geometric checks, not a
complete gameplay guarantee. The separate `tools/prepare_earth_demo.sh`
recipe applies the recorded Earth options, including this relocation, to the
specific converted-source SHA-256
`69e1f1aff0398a7042e7b158b1ac638ae3237b23d0a5a8927f0afcb7ef0c4023`.
It rejects a different input and does not overwrite an existing output. Invoke
it with `sh prepare_earth_demo.sh SOURCE_VMAP NEW_OUTPUT_VMAP`; the map and
game assets are not supplied by this repository.

On macOS arm64 the full recipe produced a local 252×576 VMAP at
`runtime/earth-map-research-2026-11-15/draft-stacked/earth-water-portal-assembled.vmap`
under TB5, SHA-256
`97f5222ab48e5987b58c12643be985e450524488bf042a9f6ae27ab9f03ea1f3`.
Compared with the prior `earth-reserved-guard-diagnostic.vmap`, the decoded
header and terrain match, the object count remains 12,249, and the sole
changed object is `monolithTwoWay_5267` at the intended new anchor. A second
run through the committed shell recipe produced identical ZIP member contents;
the ZIP file hash differed. A wrong-input check rejected the previous draft.

Forty-one focused Python geometry/conversion tests passed. The assembled map
also passed the server boat-portal check above, and a separate in-process run
checked one starting move for each of eight players plus four converted-gate
approaches with forced guard victories. Their TB5 XML outputs are
`runtime/earth-water-portal-assembled-game.xml` and
`runtime/earth-water-portal-assembled-regression.xml` respectively.

The isolated `earth-demo-profile/Maps/toroid-earth-demo.vmap` symlink now
points to the assembled map. The previous target remains locally available as
`earth-reserved-guard-diagnostic.vmap` in the same draft directory. The SDL3
client build `32936c5` with the diagnostic shortcuts started the selected map
through the normal scenario list; a human hero moved from `(164,353,0)` to
`(165,353,0)` and the client exited with code 0. The observation stream is
`runtime/earth-map-research-2026-11-15/assembled-portal-gui-observation.jsonl`.
The initial launch did not traverse the relocated portal; these checks do not
establish full scenario playability or visual acceptance.

## Graphical water portal and other exits

A separate local copy of the assembled map, SHA-256
`63253681b327921c95af3e784bb370476a17e7cac3f9da7bc142cd5fe0b28ad7`,
changes only the scenario name and the red hero's start anchor to
`(240,430,0)`. Its visitable starting tile is `(239,430,0)` on water;
game initialization creates the hero's boat. On macOS arm64, the SDL3/Metal
client build `32936c5` with diagnostic shortcuts from `17c6a69` started that
fixture as a human player. A northward move entered the relocated monolith
and arrived at `(201,145,0)` with 1,400 movement points. A screenshot shows
the hero in a boat at the exit. The hero sailed south to `(201,146,0)`,
quick-saved, sailed to `(201,147,0)`, reloaded, and sailed south again; the
recorded movement points were 1,300 after the first southward step and 1,200
after the second in both executions. A second screenshot shows the boat after
reload. The client and local server exited normally, client exit code 0.

Separately, the opt-in server check extended in `031a487` was run three times
against the assembled map with a specified different exit each time. It
reached `monolithTwoWay_5607` at `(168,290,0)`, `monolithTwoWay_10467` at
`(201,145,0)`, and `monolithTwoWay_10806` at `(169,285,0)`, preserving the
same boat ID. These are three independent staged server runs from the moved
entry, not checks of every reverse route. The graphical run covers one exit,
not normal access to the staged boat or continuous animation.

The graphical fixture, save and screenshots are local under TB5
`runtime/earth-water-portal-gui-profile/`. Observation SHA-256:
`9af69c02dfc12ddc6d0a39c217a7e7530aede2628e85162201d1e81d781262ec`;
client log SHA-256:
`fa7eaca05875e3504a5115c0f247dc974a8e7a54168a6ca35d2c47241f40e3ac`;
`after-reload-sail.png` SHA-256:
`65b9301419e7cab621dada111cd2de4e33f0f7ed3aee9002a752f392e3c92cf6`.

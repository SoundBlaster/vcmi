# Earth demo: human start and first movement

On macOS arm64, client `1.8.0.7744dbd` started the current Earth demo
through New Game, Single Scenario and the ordinary scenario list. The map
was selected using the `2.0` filter; the displayed dimensions were 252×576.
The map file had SHA-256
`61f9f042612fb86aa611c64e57b2c2e6ee7a39409cf2b4eeb1fd74b8b00d40b7`.

The run used a separate local profile with linked game data, mods and map,
copied settings, separate saves and the explicitly enabled private diagnostic
command socket. After starting the scenario, the observation stream reported
toroidal mode, player 0 and selected hero 7305 at `(164,353,0)` with 1,630
movement points. One east shortcut was accepted; subsequent observation
reported the same hero at `(165,353,0)` with 1,580 movement points.

The local profile, logs and observation stream are on TB5 under
`runtime/earth-human-current-qa/`. This verifies the ordinary human setup and
one initial movement on the current demo. It does not verify a route to the
seam, a complete turn, combat, long-term balance or animation continuity.
The run used an isolated copy of the launcher's settings, not Finder execution
of the launcher itself. The original map and game assets remain outside Git.

## Prepared seam-access draft

On 2026-11-16, the same `7744dbd` macOS SDL3/Metal client started an isolated
profile linked to the demo profile's updated map, SHA-256
`67640eefaa91e9efcf04315bef66197d834e662652bd25bd10f234391c43e5a8`. The
normal New Game → Single Scenario interface displayed the 252×576 Earth map
after filtering for `2.0`. The observation stream reported a toroidal map and
red hero 7305 at `(164,353,0)` with 1,560 movement points and 20 mana. An
east shortcut moved the hero to `(165,353,0)` with 1,510 points and 20 mana.

The profile linked game data, mods and the demo map, but had separate copied
settings and separate saves. Its local observation and screenshots are under
TB5 `runtime/earth-current-human-2026-11-16/`. The test client was forcibly
stopped after the observation, not cleanly shut down through the game UI.
This establishes the updated map's ordinary scenario-list startup and first
human-controlled move, not the complete route to the seam, graphical animation
or Finder execution of the launcher.

## Direct launch from the original start

On 2026-11-16, a separate `Start Earth Direct.command` was prepared in the
local Earth demo profile. It starts the current demo map via `--preview-map`,
leaving the existing menu-based launcher intact. The launcher was copied into
an isolated profile with linked game data, mods and the same map, and separate
settings, saves and logs. Invoked from `/Users/egor` on macOS arm64, the
`7c42081` client loaded the map; logs identified red as human. The observation
record reported a 252×576 toroidal map and selected red hero `7305` at its
original visitable position `(164,353,0)`, with 1,560 movement points.
The test process was force-stopped after observation. This verifies a shell
invocation of a copy of the launcher, not Finder double-click, clean shutdown,
continuous animation, or a complete playthrough. The isolated record is
`runtime/earth-direct-qa-2026-11-16/observe.jsonl` under the TB5 build root.
The launcher, game assets and third-party map remain local on TB5.

## Finder launch of the repaired map

On 2026-11-16, Finder opened a copy of `Start Earth Direct.command` by double
click in a newly created isolated profile. That profile linked the current
repaired demo, SHA-256
`f8add693e77bed3ee423481f9ecbb81ed9e8ddb75ecbc0d6161ca006c60e33b5`, and
game assets, with separate configuration, saves and logs. The `f44cd5a`
client process started; its profile log recorded the map load, initialized
game state and red as a human player. The test process was force-stopped.
The log is `runtime/earth-finder-direct-qa-2026-11-16/logs/VCMI_Client_log.txt`
on TB5. Finder execution and log evidence do not establish movement, a clean
shutdown, normal battle balance or visual continuity. The tested file was a
copy of the user's launcher and did not use the user's save directory.

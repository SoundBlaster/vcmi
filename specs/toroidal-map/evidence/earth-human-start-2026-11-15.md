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

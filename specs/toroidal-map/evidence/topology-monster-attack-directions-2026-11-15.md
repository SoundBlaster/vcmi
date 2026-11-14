# Direct monster attacks across seams

`AllSeamsAndLevels/ToroidalMonsterAttackTest.DirectAttackAcrossSeamUsesCorrectDefender`
exercises direct attacks across all eight edge and corner directions on each
of two levels of a synthetic 36×36 SoD map. The test relocates the hero during
setup, reveals the map, and sets the one-creature stack to be hostile and
unable to flee. These setup operations are not player actions.

For each of the 16 cases, the pathfinder selects the adjacent wrapped
destination; a real server movement request starts a battle against that
monster at its canonical tile. After a server-forced victory and acceptance
of the battle result, the monster is removed, the battle ends, and a second
movement request takes the hero from the original tile to the destination.

On macOS arm64, `vcmitest` built from `0a04fb7` plus the test change. The
16 cases and three existing edge, corner and guarded-mine tests passed three
consecutive runs. The battle result is forced rather than played out; this
does not establish negotiation, defeat, level-up dialogs, graphical combat,
remote clients, or interactions with multiple nearby guards.

## Two opposing heroes

`AllSeamsAndLevels/ToroidalHeroAttackTest.DirectAttackAcrossSeamUsesCorrectPlayers`
uses another synthetic two-player SoD fixture on the same map dimensions.
It relocates the heroes and reveals both players' maps as test setup. For all
eight directions on both levels, the pathfinder chooses the adjacent enemy;
the server starts a battle at that tile with the correct hero and player on
each side, and both players' game-state lookups identify the same battle.

The 16 hero cases, 16 monster cases and guarded-mine test passed three
consecutive runs on macOS arm64, built from `2ac006f` plus the hero test.
This stops at battle creation; actual combat, result confirmation, remote
clients and restricted player visibility were not tested in this fixture.

A complete local `vcmitest` run with the old prototype VMAP and save, plus
the Complete `Dead and Buried` H3M, passed 1,606 tests without skips; four
tests remained disabled. The log is `runtime/full-suite-combat-seams-2026-11-15.log`
under the TB5 VCMI build root. This was a local run, not hosted CI.

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
That run stopped at battle creation.

A later extension, built from `1695cb2` plus the test change, forces victory
for the attacker in each of the 16 cases. Both players in this fixture have
the same human/AI status, so the server confirms the battle result without
a replay dialog. The test checks that the battle ends for both players, the
defeated hero is removed, and a second move takes the surviving hero across
the seam onto the vacated tile. The 16 hero cases, 16 monster cases and two
AI dialog tests passed three consecutive runs. The victory is forced: actual
combat, human-versus-AI result dialogs, defeat of the attacker, remote clients
and restricted player visibility remain unverified here.

A complete local `vcmitest` run with the old prototype VMAP and save, plus
the Complete `Dead and Buried` H3M, passed 1,606 tests without skips; four
tests remained disabled. The log is `runtime/full-suite-combat-seams-2026-11-15.log`
under the TB5 VCMI build root. This was a local run, not hosted CI.

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

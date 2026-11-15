# Guarded mine approaches on both levels

On macOS arm64, the `vcmitest` binary built at `32936c5` ran
`EdgesAndBoundedControls/ToroidalGuardedMineTest.*`: all 64 parameterized
tests passed. The tests in `test/game/HeroMovementTest.cpp` exercise eight
directions on each of two levels on toroidal maps and corresponding interior
controls on bounded maps, with two battle outcomes per parameter.

The mine's approach mask permits only the cases where the direction's `dy`
is at most zero. For permitted approaches, the tests compare pathfinder and
server movement points, decline the guard dialog, revisit, accept a battle,
and check either forced victory and ownership or server-handled retreat and
neutral ownership. For disallowed approaches, they check that both a direct
path and the server move are rejected and that movement points, ownership and
visit state remain unchanged. Thus not every tested direction initiates a
battle. The server helper forces victory; the retreat follows a battle action.

The XML result is on TB5 at
`runtime/guarded-mine-directions-2026-11-15.xml`, SHA-256
`406f22f978217df477ff6ca72b1ee5fc39f52ef8e8e65396898cbaf7a6313516`.
These are server-side synthetic cases, not graphical battles, remote network
play, all mine types or a proof of all object interactions.

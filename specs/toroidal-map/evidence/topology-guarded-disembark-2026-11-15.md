# Disembarking onto a guarded shore

`ToroidalGuardedDisembarkTest.DisembarkingStartsGuardBattleAndLeavesBoatAtSea`
places an embarked hero next to an empty shore guarded by an adjacent
wandering monster. It runs across one horizontal seam and one diagonal
corner on each of the surface and underground levels. In each case the
pathfinder's direct land node comes from the water tile, and the server
accepts disembarkation with the same movement points and starts battle
against the intended guardian. Following a server-forced victory and
acknowledgement of its result, the guard is removed, the hero is on the
shore without a boat, and the same boat remains at the water tile.

The four parameterized cases and three existing embark, blocked-coast and
shore-event tests passed on macOS arm64. The test target and its dependencies
built under the checkout's TB5 build root. Logs:
`runtime/guarded-disembark-final-build-2026-11-15.log` and
`runtime/guarded-disembark-final-tests-2026-11-15.log`. Existing duplicate
library and missing Conan search-path warnings appeared during linking.

The victory is forced by the server helper; the test does not demonstrate a
graphical fight, defeat or retreat, a hidden guardian, all directions,
multiplayer, or path execution beyond the battle.

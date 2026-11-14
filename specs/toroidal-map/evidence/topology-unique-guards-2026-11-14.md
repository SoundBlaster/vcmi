# Unique guarding creatures

`CGameState::guardingCreatures` adds a monster on the queried tile before
scanning its 3×3 neighborhood. The neighborhood also includes the queried
tile, so the same monster could be returned twice. The vector is consumed by
player-visible guard queries and by pathfinding guard counts, among other
callers. The change keeps the existing discovery order and adds each object
at most once. It applies to bounded and toroidal maps.

`HeroMovementTest.GuardListIncludesEachMonsterOnlyOnce` places two monsters
at opposite horizontal edges. Before the fix, the monster on the queried tile
appeared twice on both topologies. Afterwards it appears once; the second
monster is included only on the torus, and hiding that monster from the
player removes it from the player callback result. The test uses a synthetic
map, not a live game or multiplayer session. It does not demonstrate a
specific player-visible route improvement or exhaustively test large object
footprints.

On macOS arm64, client, server, library and test targets built on TB5. A
focused run passed 27 guard, movement and Nullkiller2 tests, including battles
across an edge and a corner and checks for hidden guards.

The complete local suite passed 1,529 tests without skips, using the old
untagged laboratory VMAP and save and the Complete `Peacemaker.h3m` as the
optional external fixtures; four tests remain disabled. The log is
`runtime/full-suite-unique-guard-list-2026-11-14.log` under the TB5 build root.

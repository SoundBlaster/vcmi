# Wrapped monster battle presentation: 2026-11-10

## Finding

The server starts a battle at the defending army's `visitablePos()`.
For a toroidal map this position is canonical, including when a converted
legacy object's anchor lies outside the map. The adventure-map renderer
previously compared the battle location with the monster's raw anchor. It
therefore failed to select the directional attack image for such a monster.

## Change and verification

The renderer now uses the monster's canonical visitable position when the map
is toroidal. On bounded maps the existing anchor comparison remains unchanged.

`HeroMovementTest.WrappedMonsterBattleUsesTheCanonicalVisitablePosition`
creates a real guarded battle with a monster anchored at `(-1,-1,0)` on a
36×36 toroidal map. The battle starts at `(35,35,0)`; the test exercises the
same position predicate used by the renderer, checks a different tile and
level, and checks the bounded-map comparison. Before the fix this predicate
failed for the actual battle position; after the fix it passed. The existing
seam, corner and hidden-guard battle tests also passed. The isolated TB5
`vcmitest` and `vcmiclient` targets built successfully; the linker emitted
duplicate-library and missing Conan search-path warnings.

This confirms the position match and client compilation, not the displayed
animation or its appearance on a running GUI. Other battle render paths and
interactions between overlapping legacy objects remain unverified.

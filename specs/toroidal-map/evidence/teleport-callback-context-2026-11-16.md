# Teleport exit callback context

Review G2 suggests replacing `teleportVisitableTile(gameInfo, object)` with
`object.visitablePos()`. The getter consults the object's own callback and
returns its raw position if the callback or map header is absent. The helper
can instead normalize using the separately supplied `gameInfo` context.

`SubterraneanGateTopologyTest.heroUsesGateWithVisitableTileAcrossSeam` now
temporarily sets the exit object's callback to null after its wrapped map
occupancy has been established. Its raw visitable position is `(-1,5,1)`,
while `CGTeleport::isExitPassable` supplied with the game-state callback still
succeeds. The original callback is restored before the ordinary bidirectional
gate traversal in the same test. This is an intentionally constructed test
state, not evidence that an object loses its callback during normal play.
It demonstrates that equivalence needs an attachment/context invariant;
it does not rule out a cleanup after proving that invariant at each call site.

On macOS arm64 the test and nine related gate, whirlpool and monolith cases
passed using `0aba38b` plus the test change. XML is
`runtime/teleport-callback-review-2026-11-16.xml` under the TB5 build root.
No getter performance measurement or production change is claimed.

# Nullkiller2 direct seam routes

On 2026-11-10 a scripted-client fixture at revision `bcce542` placed an
enemy-owned synthetic resource on the opposite edge while contact between
the players was blocked. The AI looked up the destination using the hero's
anchor position, which lay outside the map, and sent a movement request
without checking the object. The new regression failed before the fix.
`AIGateway` now looks up the visitable destination tile and converts to the
hero anchor only for the movement request. Focused tests passed for toroidal
and bounded maps with contact blocked, and for a toroidal map with contact
allowed (`ai-object-coordinate-before-test.log`,
`ai-object-coordinate-after-build.log`, `ai-object-coordinate-after-test.log`
on TB5). The fixture uses a scripted client and synthetic ownership; it does
not establish a real multiplayer turn or server interaction.
The full local suite passed 1,395 tests, with three disabled
(`full-suite-ai-object-coordinate.log`).

On 2026-11-09 on macOS arm64 with copied Complete data, a fully revealed SoD
fixture checked Nullkiller2 planning for each of four straight seam directions
and four diagonal corner crossings. In each case the internal AI pathfinder
offered a same-turn path whose first destination was the wrapped neighbor;
`AIGateway` completed it with one movement request through a scripted test
client. The client applies a simulated success response, so this is not a real
server/AI game or proof of general AI strategy. Server movement is covered
separately by the [integration record](topology-demo-integration-2026-11-09.md).

During testing the scripted client was corrected to invalidate the AI path
cache on movement, as the real client does; this independent fixture change
was committed as `134a6c9`. `vcmitest` built (`ai-seam-build.log`), and
`./vcmitest --gtest_filter=Nullkiller2_AIGateway.*` passed six tests
(`ai-seam-tests.log`). A full `./vcmitest` run passed 1,333 tests with three
disabled (`ai-seam-full-tests.log`). At that revision, obstacles, boat use,
strategic choices, other AI profiles, and real client/server execution were
unverified.

Source baseline `134a6c9` with local test change:

```text
e10931c5b02bb82358be2235e3489e24cec3bae3298f747bdb8811fe218ad665  test/nullkiller2/AIGatewayMovementTest.cpp
```

## Planned interaction through the server

`ExecuteHeroChainMovementTest.plannedCornerResourceVisitUsesRealServer`
initializes a toroidal game with a hero at `(0,0)` and a gold pile at
`(35,35)`. Nullkiller2's own pathfinder supplies a same-turn plan whose first
target is the wrapped corner. `ExecuteHeroChain` sends one movement request
through `GameHandlerTestClient`, which deep-copies it and calls the real
server request handler. The server removes the resource and credits 1,000
gold while the hero remains on the starting tile, as for a blocking visit.

This test and six related AI movement tests passed three consecutive runs on
macOS arm64. Client and server share the fixture's game state in one process;
the test does not exercise a real network connection, the full strategic
turn loop, fog exploration, or other objects and corners.

The server-backed resource-visit fixture was subsequently expanded to all
four straight seams and four corner directions on both map levels. In each
of the 16 configurations, Nullkiller2 planned a same-turn direct visit and
the server handler removed the gold pile and credited the player after one
movement request. All 16 cases passed three consecutive runs on macOS arm64.
The game state remains shared in-process, the map is fully revealed, and the
test does not establish a full AI turn or graphical multiplayer behavior.

A separate parameterized fixture uses the same 16 directions and levels with
an empty destination. Nullkiller2 plans the direct step, `ExecuteHeroChain`
sends one request to the real server handler, and the hero appears at the
wrapped destination with movement points spent. Both parameterized fixtures
passed all 32 cases in three consecutive runs. This checks actual movement,
but still uses shared in-process game state and a fully revealed map.

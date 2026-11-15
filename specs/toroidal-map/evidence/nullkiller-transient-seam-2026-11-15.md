# Nullkiller2 transient movement across seams

The parameterized `ToroidalHeroChainTest` builds a 36×36 two-level toroidal
map with an isolated start and destination separated by water. It grants the
hero a permanent Water Walk or Flight bonus, requests a same-turn Nullkiller2
plan and executes it through `ExecuteHeroChain` and an in-process
`CGameHandler`. The test records client movement requests and checks that the
water tile is sent in the respective WATER or AIR layer, the hero reaches the
other shore in the LAND layer, and movement points decrease.

Each ability is tested across all eight edge and corner directions on both
levels, giving 32 scenarios. The earlier 32 planned movement and resource
visit cases were run alongside them. All 64 passed on macOS arm64 with test
binary built from `210e6f7` plus the tests committed in `2043e46`; results are
`runtime/ai-transit-all-2026-11-15.{log,xml}` under the checkout's TB5 build
root. The build reported duplicate-library and missing Conan search-path
warnings.

This initial run used a shared-state server fixture, not a remote multiplayer
or graphical session. It did not test the AI casting the spells, undiscovered
landing tiles, multi-turn transit, interactions that interrupt landing, or
autonomous choice of this route during a full AI turn.

## Return after server-driven transit

`ToroidalAITransientSafetyTest.ReturnsToIslandAfterServerBackedTransit`
starts on an isolated island with 1,200 movement points. A test client
submits six individual Flight or Water Walk steps west across the seam to an
in-process `CGameHandler`. The server rejects a seventh step, preserving the
hero's position and points. Nullkiller2's gateway then returns the hero to the
island through real movement requests, ending in the LAND layer. The test
covers both active bonuses and both map levels.

The gateway also declines the unsafe water tile as an explicit destination
without sending a movement request. This is not evidence that it applied the
landing-safety filter: its automatic destination selection considers LAND and
SAIL nodes, and therefore also declines the otherwise safe water destination.
The first six moves are test-driven, not autonomously selected by the AI.

The four new cases passed together with the earlier 64 AI seam movement and
resource cases and two pathfinder/server parity cases: 70 tests in total on
macOS arm64. The binary was built from `56a83d9` plus the test committed in
`c11185d`. Reports are `runtime/ai-transit-return-final-2026-11-15.{log,xml}`
under the TB5 build root. This does not cover automatic spell casting, dynamic
fog, a full autonomous AI turn, a remote server or graphical play.

## Planned spell casts and rejection

The subsequent `ToroidalHeroChainTest` cases give the hero a spellbook with
Flight or Water Walk, adequate spell skills and mana, and place the destination
on the far shore with water between it and the start. Nullkiller2's pathfinder
must produce a same-turn route containing an adventure-spell cast action. The
test explicitly selects and executes that planned route through
`ExecuteHeroChain`; it does not ask the AI to choose its overall turn goal.
An in-process server receives serialized requests and returns actual request
acknowledgements to the gateway. For each ability and all eight seam and
corner directions on both levels, the cast succeeds, mana decreases, a
server-accepted AIR or WATER movement crosses the seam, and the hero reaches
land. This covers 32 scenarios.

For another 32 scenarios, the test sets the hero's mana to zero immediately
before the server receives the cast, simulating a change after planning and
local checks. The server rejects the cast, the gateway observes the failed
acknowledgement, and the hero remains at the start with no movement request or
new movement bonus. The mana change itself is injected by the fixture; no
actual gameplay event causing it is exercised.

All 64 new cases passed alongside the 68 earlier AI seam and return tests:
132 tests in two suites on macOS arm64. The binary was built from `db9a590`
plus the tests committed in `9a94d67`. Reports are
`runtime/ai-cast-transit-negative-2026-11-15.{log,xml}` under the TB5 build
root. The run does not cover autonomous goal selection, other effects or
obstacles, undiscovered landing tiles, interruptions, remote clients or GUI.

## Autonomous planning turn for a visible resource

`ToroidalAITransientSafetyTest.FullTurnCastsToCollectVisibleGoldAcrossSeam`
runs `Nullkiller::makeTurn()` with a spell-equipped hero on a small island and
a visible pile of gold on land across the west seam. A water tile separates
the island from the opposite shore. The AI itself chooses its task, plans and
casts Flight or Water Walk, crosses using the corresponding transient layer,
and collects the gold. The test checks the server's acknowledgement, a
successful seam movement, resource removal, the 1,000-gold increase and the
hero's final LAND layer. It covers both spells on each of the two map levels.

The in-process server applies real serialized requests to a shared game state.
The adapter forwards movement and object-removal notifications to Nullkiller,
and supplies the server's request acknowledgements. It does not reproduce
every client callback or independent network replica. `Nullkiller::makeTurn()`
is the AI planning and action loop; the test does not exercise the outer
network turn handoff. All tiles are revealed, and no battle or GUI is involved.

All four new scenarios passed alongside the earlier AI seam tests and related
turn tests: 139 tests in four suites on macOS arm64. The binary was built from
`1ebc19a` plus the test in `0462b36`; reports are
`runtime/ai-full-turn-transit-related-2026-11-15.{log,xml}` under the TB5 build
root. This result does not establish autonomous spell use in other directions,
under fog, with an interrupted landing, or in a remote or graphical session.

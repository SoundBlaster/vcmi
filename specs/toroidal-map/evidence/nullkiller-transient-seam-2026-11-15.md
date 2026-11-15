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

This is a shared-state server fixture, not a remote multiplayer or graphical
session. It does not test the AI casting the spells, undiscovered landing
tiles, multi-turn transit, interactions that interrupt landing, or autonomous
choice of this route during a full AI turn.

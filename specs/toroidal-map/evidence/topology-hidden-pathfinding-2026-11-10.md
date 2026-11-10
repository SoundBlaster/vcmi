# Hidden terrain in wrapped pathfinding

The pathfinder ran against authoritative game state while generating a
player's routes. On a toroidal map, a hidden destination could leave different
node accessibility values depending on its actual terrain. A diagonal sailing
route could also have a different cost depending on whether an unseen
orthogonal neighbor was water or coast. Both differences were reproduced with
synthetic fog-of-war states; they are not evidence of a leak in a particular
GUI or network session.

For toroidal route planning, neighbor enumeration now ignores unrevealed
destinations and conservatively excludes a diagonal sailing step until both
orthogonal neighbors are known. The server's physical movement check still
uses actual terrain. Bounded-map enumeration retains its previous behavior.

Two player-callback pathfinding tests compare hidden rock versus grass and
hidden water versus land, including reachability, node accessibility and route
cost. They also check the change when terrain becomes visible, rejection of
an actual rock crossing, and acceptance of a physically legal diagonal boat
move with an unknown side cell. These synthetic single-player checks do not
establish complete fog confidentiality, all knowledge sources, actual client
presentation, or network secrecy.

On macOS arm64 the `vcmitest`, `vcmiclient`, `vcmiserver`, and `vcmieditor`
targets built. The two focused tests passed. The wider test run passed 1,444
tests, skipped three external-input cases, and reported five disabled tests.
It excluded `HeroMovementTest.TcpPeersReceiveAuthoritativeCornerMovement`
because an earlier instance remained hung; this run does not verify that
network case. Its log is `full-suite-pathfinding-fog.log` on TB5.

## Hidden guards

A later synthetic test placed a hidden monster across a seam from a visible
tile. With the monster present, the player's route previously exposed both
`GUARDED` accessibility and a battle action; removing it changed those values.
On toroidal maps, pathfinding now considers only guards whose visitable tiles
are known to that player's team. It checks all guards when the cached first
guard is hidden, so a second, visible guard remains effective. Bounded maps
continue to use the existing guard queries. The actual server battle logic is
unchanged.

The test compares accessibility, reachability, actions and movement values
across the map with a hidden guard and with that guard removed, and checks that
revealing it restores the guarded action. A separate test covers a visible
guard alongside the hidden cached guard. A server test places the corner
guard under fog and verifies that entering its guarded tile starts battle with
that guard. These are synthetic single-player state checks, not a claim about
normal sight evolution, all guard interactions, AI behavior, a GUI, or network
confidentiality.

The client, server, editor, and test targets built on macOS arm64. The wider
run after the pathfinding change passed 1,446 tests, skipped three external
input tests, and reported five disabled tests; it excluded the still-hung TCP
case. Its log is `full-suite-hidden-guards.log` on TB5. The added server-battle
test subsequently passed in a focused run together with four guard tests.

## Player guard callbacks

`WrappedPlayerGuardQueriesDoNotRevealHiddenGuards` reproduced a separate
player-callback leak: a visible tile next to two hidden monsters across the
seam returned their objects and a hidden guard position through three callback
methods. These APIs are used outside the ordinary pathfinder, including by
Nullkiller2. The test also covers a visible second guard when the cached first
guard is hidden.

On toroidal maps, a player callback now returns only visible guards and uses
the first visible one when the cached guard is hidden. The position query
returns an invalid position for an unseen tile. Unrestricted callbacks and
authoritative game state retain the actual guard, and the test checks that
bounded-map callbacks retain their previous behavior. The existing server
test still starts a battle against a hidden guard across a corner. This is
not a complete audit of AI, other `Unchecked` methods, spell cursors, or
network confidentiality.

The client, server, editor and test targets built on macOS arm64; linker
warnings about duplicate libraries and missing Conan search paths remain.
Eleven focused guard tests passed. The complete enabled suite passed 1,449
tests, skipped three optional external-input tests and reported five disabled
tests; no TCP case was excluded. Logs are `hidden-callback-before-test.log`,
`hidden-callback-final-tests.log` and `full-suite-hidden-callback.log` in the
TB5 build root. These are synthetic tests, not a graphical AI playthrough.

## Dimension Door guard cursor

With guarded Dimension Door landings enabled, the cursor used an unchecked
guard query. A synthetic test showed the guarded cursor on a visible seam
tile even when both adjacent monsters were hidden; it also exposed a guard
on an unseen destination when `exposeFow` was enabled. The test reproduced
the first case with both player and unrestricted callbacks.

For toroidal maps, the cursor now requires a visible destination and a guard
visible to the caster's owner. It still displays the guarded cursor when a
second guard is visible, including through an unrestricted callback. The test
also checks that the bounded cursor retains its prior behavior. It does not
establish that every spell-target validation path, mod configuration, or GUI
interaction preserves fog confidentiality.

The client, server, editor and tests built on macOS arm64. Twenty-one focused
guard and spell tests passed, as did 1,450 tests in the complete enabled suite;
three optional external-input tests were skipped and five tests remain
disabled. No TCP test was excluded. The test and suite logs are
`hidden-cursor-final-tests.log` and `full-suite-hidden-cursor.log` on TB5.

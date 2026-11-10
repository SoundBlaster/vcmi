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

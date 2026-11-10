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

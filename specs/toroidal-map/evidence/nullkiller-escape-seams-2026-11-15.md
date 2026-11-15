# Nullkiller escape across toroidal seams

`EdgesCornersAndLevels/ToroidalAIEscapeTest.FullTurnEscapesEnemyAcrossSeam`
constructs a 36×36 two-level map with a narrow land corridor. A weak AI hero
starts beside a much stronger visible enemy, at an edge or corner. The only
long land route away from that enemy begins across the seam. The test uses an
artificial 3,000 movement points to make a safe destination reachable; it is
not evidence of typical game balance.

The gateway scans and remembers visible objects. A planning pass confirms an
immediate threat and finds an escape goal whose destination is not reachable
by the enemy in its current predicted turn. The test then runs
`Nullkiller::makeTurn`. Requests are serialized and delivered to the real
in-process `CGameHandler`; the test server applies packets, returns request
acknowledgements and notifies the AI gateway. The assertions require the
server to apply the first wrapped step and a movement to the previously
selected escape destination, preserve the enemy's position and leave the
hero on land.

Sixteen combinations cover four cardinal crossings and four diagonal corner
crossings on each level. The new cases and related threat, fog and escape
tests passed five runs of 43 tests on macOS arm64, Release build based on
`76a27d6` with the test change. The TB5 artifacts are
`runtime/nullkiller-escape-all-seams-final-2026-11-15.log` and the adjacent
XML file. The build retained existing duplicate-library and missing Conan
search-path warnings.

The test uses synthetic terrain, manually sets movement points and invokes
the AI planning turn after a separate preflight planning pass. It does not
exercise the graphical client, an external network, the gateway's end-turn
operation, or all defence and escape situations. Reaching the same
destination supports the connection between planning and the played move;
the test does not instrument the identity of the task selected inside the
AI's priority loop.

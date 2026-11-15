# Simultaneous-turn contact at map seams

`HeroMovementTest.SimultaneousTurnContactRecognizesMeetingAcrossToroidalSeams`
uses the actual server turn-order processor on a generated two-level 36×36
map. Nearly all terrain is rock; two heroes can meet only on a single open
tile across an edge or corner. For each of four directed edge arrangements
and four corners, on both levels and with bounded and toroidal topology, it
checks both heroes' pathfinder access to that tile and the resulting
`SimturnsContactStatus` packet. The test server applies the packet to the
game state and then interrupts startup before any turns begin. On toroidal
maps the meeting is possible and contact is allowed; on the bounded control
the isolated players retain simultaneous turns with contact blocked. A
separate control puts the heroes on the same coordinates on different levels
without a gate, and checks they remain separate on both topologies.

`BoundedAndToroidal/SeamSimultaneousTurnTest` additionally lets the real
processor start and end turns for one horizontal arrangement. On the bounded
map both players start simultaneously; on the toroidal map only the first
starts because the players can meet. Ending that player's turn leaves the
second player acting in both cases. These tests use an in-process test server,
fully revealed terrain, a forced-turn minimum of zero and a maximum of 99 days;
they do not exercise packets on TCP, graphical clients, changes during a
running day, AI decisions, boats or every possible obstacle arrangement.

On macOS arm64, `vcmitest` built from `746d247` plus the tests, and all three
focused tests passed. The XML report is
`runtime/simturn-seams-final-2026-11-15.xml` on the TB5 build root. The build
completed with duplicate-library and missing Conan search-path warnings.

# Nullkiller threat planning and fog perspective

Nullkiller's danger hit map builds paths for visible enemy heroes. Its node
storage evaluates accessibility using the AI player's fog, but the shared
pathfinder helper had used the simulated hero owner's fog for toroidal
neighbor and known-guard checks. In a synthetic two-player position with a
visible enemy and target on opposite sides of an edge, keeping the target
hidden only from the enemy caused the AI to report no immediate threat.
The ordinary bounded control detected an adjacent interior tile. The failing
focused result is `runtime/nullkiller-danger-opponent-fog-before-2026-11-15.xml`
under the TB5 build root. This deliberately narrow enemy visibility isolates
the information dependency; it is not a claim that such adjacent fog occurs
in normal play.

Pathfinder options now allow a knowledge observer, defaulting to the hero's
owner. Nullkiller sets it to its own player ID; the hero's actual owner is
still retained for movement and diplomacy. Toroidal neighbor and known-guard
lookups use that observer. The existing node storage already uses the same AI
player's fog. A subsequent change applies that perspective to teleport
channel visibility; its scope is recorded in the
[teleport observer evidence](nullkiller-teleport-observer-2026-11-15.md).
These checks do not prove every AI subsystem obeys the same boundary.

`Nullkiller2_DangerAcrossSeam` uses a revealed AI map and a remembered visible
enemy with deliberately restricted enemy fog. It checks both directions of
the four cardinal seams and the four corner directions on surface and
underground: each bounded control cannot reach the target in the current
turn; on the torus the threat map identifies the enemy and a current-turn
threat. A fresh AI with the target hidden from it and revealed to the enemy
does not report danger there. The test inserts the visible enemy into AI
memory directly; it does not exercise live discovery or an autonomous
defence decision.

`Nullkiller2_PathfinderKnowledge` reverses two teams' visibility of a guard
and compares known-guard and neighbor queries with an explicit observer and
the default owner. The two existing hidden-guard pathfinding checks also
passed. The 19 focused cases passed on macOS arm64, Release build based on
`7557caa` with this change. Results are in
`runtime/nullkiller-threat-knowledge-final-2026-11-15.xml` on TB5. The build
reported existing duplicate-library and missing Conan search-path warnings.

The full local suite then passed 1,944 tests, skipped four optional external
map, save and legacy-server fixtures, and reported three disabled tests. The
log and XML are `runtime/full-suite-nullkiller-knowledge-2026-11-15.log` and
`runtime/full-suite-nullkiller-knowledge-2026-11-15.xml` on TB5. Client,
server and Nullkiller2 targets built, with the same linker warnings; these
builds are not a runtime game check. `specpm validate . --json` reported no
errors or warnings.

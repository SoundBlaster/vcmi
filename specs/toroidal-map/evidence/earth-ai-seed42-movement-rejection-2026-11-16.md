# Earth AI run: movement layer rejection

On macOS arm64, an isolated headless game used the mine-repaired Earth demo
SHA-256 `4b3c264a4cd7f9580c7161da572f9e1a06ebb3a26aef368ff3d9f95e7b3f8832`
with `--testmap`, `--testseed 42` and AI controlling all players. The client
reported the requested seed. No forced victories or fog reveal were applied.
After 45 seconds the process was terminated by the test harness during the
orange player's turn on day 27. This was not a finished game, and the seed
alone does not establish deterministic scheduling of all AI activity.

On day 24 the blue AI attempted a hero exchange. Its route sent hero Jeddite
from hero anchor `(55,362,0)` toward `(54,363,0)`. The server rejected the
`MoveHero` request with `Hero cannot use the requested movement layer!` and a
failed movement packet; the AI abandoned that goal and replanned. Play
continued through subsequent turns, and the same hero later moved elsewhere.
The logged coordinates are away from the map seam. The trace does not record
the submitted layer or establish why the route and server disagreed, whether
the other hero occupied a boat, or whether the cause is specific to wrapping.
The rejection is a concrete pathfinder/server agreement issue to reproduce,
not proof that the map or AI always fails.

The stdout also contains one `Repeated interaction did not move the hero`
message and other AI subgoal failures followed by replanning. No
`Cannot disembark hero, tile is blocked!` message appeared in this run. These
observations are not a comprehensive AI audit. Captured stdout and detailed
client/server trace are under
`runtime/earth-mine-autonomous-seed42-2026-11-16/` on TB5; the trace is large.
The executable logged version stamp `8e80ec5` and was the locally built
client used for the preceding preview check.

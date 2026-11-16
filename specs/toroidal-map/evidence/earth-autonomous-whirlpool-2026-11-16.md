# Earth: autonomous play and whirlpool placement

An isolated `--headless --testmap` run of the previous Earth demo let all
players use AI without forced victories or fog-reveal interventions. The run
was stopped after 45 seconds; it reached the tan player's turn on day 27. It
did not establish that a complete game could finish. The local run log is at
`runtime/earth-autonomous-qa-2026-11-16/stdout.log` under the TB5 build root.
No explicit random seed was recorded, so this playthrough is not a
deterministic reproduction.

Four attempted whirlpool trips failed with `Cannot disembark hero, tile is
blocked!`, all selecting destination hero anchor `(5,266,0)`. Inspection of
the prior map identified `whirlpool_10403` with six visitable cells; its
three cells on row 266 were snow. All six corresponding visitable cells in
the original converted map were water. The other nine Earth whirlpools had
only water visitable cells in the previous demo.

The preparation recipe now explicitly moves this whirlpool one row north,
from anchor `(4,266,0)` to `(4,265,0)`. It requires the source entrances and
destination footprint to be water, rejects other occupying objects, and
checks for a clear water approach. The source map is unchanged. Comparison of
the prepared archives found `objects.json` was the only changed member and
the whirlpool was the only changed object. This is a demo placement repair,
not a change to general whirlpool or boat rules.

## Verification

- All 28 Python assembler tests passed, including rejection of dry or occupied
  destinations and a dry source. The Earth recipe and the existing road
  preparation test completed on macOS arm64.
- `MapFormat.InitializesExternalToroidalMapAndChecksPortalPairs` with
  `VCMI_INSPECT_EARTH_WHIRLPOOL_WATER=1` reproduced the issue on the prior map:
  the engine's exit list included `(2,266,0)`, `(3,266,0)` and `(4,266,0)`
  on land. On the repaired map, it returned six water exits. The server then
  accepted direct `MONOLITH` movements of a hero in a boat to each exit and
  the hero remained aboard. This is a staged server check, not six complete
  player-initiated whirlpool dialogs.
- The same engine and server check passed on the seam preview.

## Post-fix autonomous run

A separate unseeded AI-only, headless run used the repaired map and no forced
victories or fog reveal. It was stopped after 45 seconds, during pink's turn
on day 29. The captured stdout contains no `Movement request for hero` failure
and no `Cannot disembark hero, tile is blocked!` message. The full server trace
records hero Rissa travelling from hero anchor `(251,357,0)` to `(5,264,0)`,
and later from `(204,231,0)` to `(5,265,0)`. Both responses have result `2`
(`TryMoveHero::TELEPORTATION`), placing the visitable position on the repaired
whirlpool's two different rows. The game continued after these transitions.
Logs are under `runtime/earth-autonomous-whirlpool-repair-2026-11-16/` on TB5.
This is one random partial game, not a complete playthrough or evidence of
visual animation continuity.

The server trace also records two ordinary successful (`TryMoveHero::SUCCESS`)
diagonal westward seam steps by the same AI hero, with hero anchors
`(1,353,0) → (252,354,0)` and `(1,260,0) → (252,259,0)`; the matching
movement packets were delivered to the client. Hero anchors use the game's
offset from visitable positions, so an anchor x of 252 is consistent with a
visitable x of 251 on this 252-wide map. These are two naturally selected
steps in this run, not coverage of both directions, the vertical seam or
corners in an unassisted game.

The same stdout also reports AI subgoal failures followed by replanning,
including four `Repeated interaction did not move the hero` messages for
hero-exchange tasks near towns, as well as lost-hero and unreachable-target
messages. The repeated-interaction safeguard deliberately stops retrying a
previously idle interaction; these observations do not establish why the
exchange failed or whether topology is involved. They are retained as an
AI limitation rather than counted as server movement rejections or silently
treated as a clean AI run.

The repaired local map SHA-256 is
`ea6c4de705dba4139f2ceb780a7bc7a747c3075c2244188980d50caf16b89819`;
the seam-preview SHA-256 is
`e14da15866653f5bf585d7b580efa2e83b64d14eba3466298c01b24652d2d64f`.
Both launch profiles now use those maps. The previous demo map remains at
`earth-portal-approach-2026-11-16.vmap`, and the previous preview is kept as
`earth-seam-preview-before-whirlpool-2026-11-16.vmap.bak`. Game assets and
third-party map data remain outside the repository.

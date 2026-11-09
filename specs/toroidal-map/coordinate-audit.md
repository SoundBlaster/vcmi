# Coordinate and distance audit

This is a review queue for the toroidal feature, not a claim that every match
is a bug or that every topology-dependent expression has been found. The
[candidate index](coordinate-audit-index.md) records per-file search results
and review status at baseline `27dff9a`; `partial` means only the specifically
named observation was examined. Remaining lines in that file still need review.

## Reproduce the search

From the repository root:

```sh
python3 specs/toroidal-map/tools/audit_coordinates.py --summary
python3 specs/toroidal-map/tools/audit_coordinates.py --category distance_or_neighbor
python3 specs/toroidal-map/tools/audit_coordinates.py --path lib/mapObjects/MiscObjects.cpp
```

The default output is JSON Lines with a path, source line, matching categories
and source text. The tool scans tracked C++ files under `lib`, `client`,
`server`, `AI` and `mapeditor`. Patterns include distance and neighbor method
names, map-boundary predicates, coordinate arithmetic, common math functions
and extent/border calculations. Categories overlap. At this baseline the scan
matched 2,067 lines in 346 files: 460 distance/neighbor, 113 boundary, 1,015
coordinate arithmetic, 28 coordinate math and 530 extent/border lines. Counts
are matches, not findings. The scan includes battle and UI coordinates,
generator internals, declarations, comments and third-party code. It excludes
tests and any code outside the named scopes.

The patterns cannot identify type or data flow, and can miss aliases, macros,
multiline expressions, indirect calls, operators and names not covered by the
expressions. `clang-query` was unavailable on this machine and the existing
CMake build did not emit `compile_commands.json`; no AST audit was performed.
The index may be regenerated with `--markdown-index`, but doing so resets
manual review statuses. Preserve or reconcile those statuses before replacing it.

## Initial triage

| Area | Observation | State |
| --- | --- | --- |
| Hero Visions | Spell range used ordinary Euclidean distance; it now uses wrapped displacement when applicable. | Fixed and game-state tested; [evidence](evidence/topology-visions-spell-2026-11-09.md). |
| Nearest-town spell effects | Town Portal and Reinforcements shared a rectangular distance helper. | Helper fixed; bounded and toroidal Town Portal tested; Reinforcements and cross-level casts remain unverified. [Evidence](evidence/topology-nearest-town-2026-11-09.md). |
| AI Town Portal | `AI/Nullkiller2/Pathfinding/AINodeStorage.cpp` independently selects the nearest town. | Planner now uses wrapped distance; bounded and toroidal same-turn plans tested. Live AI casting and candidate-pool differences remain unverified. [Evidence](evidence/topology-ai-town-portal-2026-11-09.md). |
| Underground gates | `lib/mapObjects/MiscObjects.cpp` pairs gates on different levels using 2D distance. | Pairing uses wrapped horizontal distance on toroidal maps; synthetic channel-ID regression passes. Actual channel membership and traversal remain unverified. [Evidence](evidence/topology-gate-pair-2026-11-09.md). |
| Grail placement | `lib/gameState/CGameState.cpp` restricted the placement area using a fixed border and rectangular radius. | Toroidal placement includes eligible edge tiles and uses wrapped radius; the unspecified-location sentinel is handled separately. Bounded and focused toroidal cases tested. [Evidence](evidence/topology-grail-placement-2026-11-09.md). |
| AI startup and object clusters | Startup uses a rectangular distance threshold; cluster center uses coordinate averaging and distance. | Candidates: check actual behavior and select a toroidal center algorithm rather than replacing individual arithmetic mechanically. |
| Ranged adventure spells and Dimension Door | `AdventureSpellEffect.cpp` checks range using rectangular coordinate differences; `AINodeStorage.cpp` clips Dimension Door landing enumeration to map edges, and `ObjectClusterizer.cpp` has a separate rectangular reach heuristic. | Candidates: align spell validation and AI candidate generation with topology while preserving visibility, landing and level rules. |
| Random map generator | Many matches in `lib/rmg` affect zone placement and connectivity. | Unreviewed; generator support for toroidal maps is conditional and has not been claimed. |
| Battle and interface layout | Many matches address battle hexes, pixels or widgets instead of the adventure map. | Classify before editing; do not globally change `int3` or every matching method. |

Review authoritative gameplay and persistence first, then the player interface,
supported AI, editor and any generator profile. For every candidate, record
whether topology applies, what changes, and which behavior was tested. Keep
the acceptance matrix pending where a subsystem has only partial coverage.

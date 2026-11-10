# Coordinate and distance audit

For the map flag, legacy compatibility and the role of the shared topology
layer, see [topology design](topology-design.md).

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
| Underground gates | `lib/mapObjects/MiscObjects.cpp` pairs gates on different levels using 2D distance. | Pairing uses wrapped horizontal distance; initialized synthetic gates connect and support server-side traversal in both directions. Original graphics and other passages remain unverified. [Evidence](evidence/topology-gate-pair-2026-11-09.md). |
| Grail placement | `lib/gameState/CGameState.cpp` restricted the placement area using a fixed border and rectangular radius. | Toroidal placement includes eligible edge tiles and uses wrapped radius; the unspecified-location sentinel is handled separately. Bounded and focused toroidal cases tested. [Evidence](evidence/topology-grail-placement-2026-11-09.md). |
| Grail holes | Hole positions are stored separately from tile occupancy and could have raw anchors outside the rectangle. | Grail exclusion and hole lookup now recognize their wrapped cells; targeted state and in-memory restore tests pass. [Evidence](evidence/topology-grail-placement-2026-11-09.md). |
| Summon Boat and boat placement | Nearest boat used planar Manhattan distance, and `IBoatGenerator::bestLocation` rejected off-rectangle neighbors before considering topology. | Both now account for wrapped positions; bounded and toroidal server-side casts tested for nearest boat and a destination across a seam. The second issue was missed by the baseline search. [Evidence](evidence/topology-summon-boat-2026-11-09.md). |
| Object tile queries | `visitableAt`, `blockingAt`, `coveringAt` and `drawLayerAt` compared canonical tile positions to an unwrapped object anchor; `visitablePos` could return a cell outside the rectangle. | Footprint queries and visitable positions now use topology when a toroidal map is attached. Synthetic corner, resource, gate and Town Portal tests pass; seam-crossing static footprints fitting one period can be saved. [Queries](evidence/topology-object-tile-queries-2026-11-10.md), [round trip](evidence/topology-wrapped-footprint-vmap-2026-11-10.md). |
| Unrestricted object visibility | A callback without a player checked raw bounding cells against the rectangle, hiding objects whose entire footprint wrapped across a seam. | The unrestricted check now uses map topology; bounded behavior, fog isolation and invalid level are covered by a focused callback test. [Evidence](evidence/topology-unrestricted-object-visibility-2026-11-10.md). |
| Position-targeted objectives | Objective resolution checked raw map bounds and looked up raw coordinates. | Lookup wraps valid positions and anchors on toroidal maps; five town-targeted condition types and bounded/sentinel cases are tested. Editor movement and complete objective execution remain open. [Evidence](evidence/topology-wrapped-objectives-2026-11-10.md). |
| Town appearance terrain | `CTownInstanceConstructor` read terrain directly at the object anchor, which may be outside the map rectangle in a toroidal map. | The terrain lookup now wraps its position for toroidal maps; a real town constructor is tested across both axes and multiple periods, with bounded behavior checked separately. [Evidence](evidence/topology-town-anchor-terrain-2026-11-10.md). |
| Editor terrain and roads | Editor road painting clipped the northern source tile, and the terrain redraw region did not cross map seams. | Road lookup and redraw regions now respect topology; extracted coordinate calculations are tested, while Qt painting and interactive redraw are unverified. [Evidence](evidence/topology-editor-road-redraw-2026-11-10.md). |
| Ambient sound sources | `MapAudioPlayer` clipped non-visitable object and terrain-overlay source cells at the rectangle even after its attenuation calculation had been wrapped. | Source cells are canonicalized and deduplicated; bounded and toroidal object cases are tested, but audible playback is unverified. [Evidence](evidence/topology-audio-2026-11-09.md). |
| AI startup and object clusters | Startup used a rectangular distance threshold; cluster center used coordinate averaging and distance. | Startup threshold now uses topology-aware distance; a synthetic game-state fixture compares bounded and toroidal behavior. Cluster representative uses a weighted circular center and wrapped distance; a real-object fixture tests both topologies, weights and ambiguity. [Startup evidence](evidence/topology-ai-startup-2026-11-09.md), [cluster evidence](evidence/topology-ai-cluster-center-2026-11-09.md). |
| AI interaction precheck | `AIGateway` looked up the hero anchor instead of the destination tile, which could skip an enemy-object check at a seam or on a bounded map. | Corrected and exercised with a scripted client, synthetic object and blocked/allowed contacts. [Evidence](evidence/topology-ai-direct-route-2026-11-09.md). The original coordinate scan missed this conversion. |
| AI patrol radius | The shared pathfinder calculates patrol bounds through `getTilesInRange` using Manhattan distance. | The wrapped callback and real path calculation cover one corner patrol with bounded comparison; live AI behavior remains unverified. [Evidence](evidence/topology-ai-patrol-2026-11-10.md). |
| Ranged adventure spells and Dimension Door | `AdventureSpellEffect.cpp` used rectangular coordinate differences; AI clipped landing and exploration enumeration and used a rectangular cluster reach heuristic. | Spell range, AI enumeration and reach heuristic fixed and tested; server casts across one edge and one corner passed. GUI casts and assembled cluster selection remain unverified. [Evidence](evidence/topology-ranged-spells-2026-11-09.md), [exploration](evidence/topology-rectangle-enumeration-2026-11-09.md). |
| Random map generator | Many matches in `lib/rmg` affect zone placement and connectivity. | Unreviewed; generator support for toroidal maps is conditional and has not been claimed. |
| Battle and interface layout | Many matches address battle hexes, pixels or widgets instead of the adventure map. | Classify before editing; do not globally change `int3` or every matching method. |

Review authoritative gameplay and persistence first, then the player interface,
supported AI, editor and any generator profile. For every candidate, record
whether topology applies, what changes, and which behavior was tested. Keep
the acceptance matrix pending where a subsystem has only partial coverage.

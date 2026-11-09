# Toroidal adventure map: review and evidence

## Status and consumer

This is a local, noncanonical **draft contract**, not a description of a working
feature. The consumer is a player and map author who needs an adventure map
whose north/south and east/west edges join, with traversable diagonal corners
and seamless scrolling. The user confirmed this goal and the earlier assumption
that surface and underground wrap independently. That confirmation is not an
approval of every derived requirement or implementation choice in this draft.

The source baseline is VCMI `develop` commit
`30e9207276f137658da5132705c05644bfe33aee`, checked out clean before this
specification work. Only selected entrypoints are analyzed. The user request
and confirmation exist in the task conversation, not as a separate signed or
versioned requirements document. This file records their substance.

**Evidence levels:** U = user-stated or user-confirmed goal; S = source reading
on the pinned baseline; D = design obligation inferred from that goal; A =
analytical example; R = runtime observation. There is no R evidence for this
feature. Structural validation checks artifact format, not gameplay.

The [implementation matrix](implementation-matrix.md) tracks requirements,
scenario coverage, implementation, verification, and acceptance separately.
Scenario definitions below are acceptance proposals; their existence is not a
test run.

## Boundary and composition

The subject is the assembled adventure map session. Existing VCMI responsibilities
are mapped in `architecture.hc` and `architecture.hcs`; composition is not a
claim about call order. The map state owns canonical locations; route finding
proposes steps; the server decides whether movement is accepted; the client
renders and converts input; visibility, guards, AI and persistence consume
positions. The assembly must use one consistent topology and level identity
across those roles. No single child alone guarantees this outcome.

For a toroidal level, a canonical coordinate is `(x, y, z)` with
`0 <= x < W`, `0 <= y < H`, and a valid level `z`. Repeated images in a
viewport are presentation locations, not additional game-state objects.
Crossing each axis changes the canonical coordinate modulo its positive extent;
the level does not change. A seam does not grant passage through a blocked
tile, bypass normal diagonal movement rules, reveal hidden information, or
create a duplicate hero. Ordinary bounded maps retain their baseline behavior.

## Claim and assembly matrix

| Consumer question | Draft obligation / origin | Relevant current contribution | Assembly, evidence and conflict | Review owner decision |
| --- | --- | --- | --- | --- |
| Can a hero cross any edge and corner? | `topology`, `level_isolation` (U); `movement_and_route`, `authority` (D) | `CPathfinderHelper::getNeighbours`; `CGameHandler::moveHero`; `CMap::isInTheMap` | S: candidates outside the rectangle are discarded; the server checks bounds and ordinary `areNeighbours`. Existing behavior conflicts with the goal. Route and server must agree on wrapped adjacency and normal movement rules. | Retain the same movement rules at seams; review exact tiny-map behavior. |
| Can the player continuously see and click across a seam? | `rendering`, `viewport_input`, `movement_presentation` (U/D) | `MapViewController::setViewCenter`, `MapViewModel::getTileAtPoint`, `MapRenderer::renderTile`, `MapViewActions` | S: clamped camera, border tiles and input bounds checks. D: use consistent presentation-to-canonical mapping; a visible repeated image is not a second entity. Animation interpolates unwrapped endpoints. Pixel-level quality is unobserved. | Confirm interaction and visual criteria; choose camera/render design. |
| Are encounters and visibility fair at seams? | `vision_and_guards` (D) | `CGameInfoCallback::getTilesInRange`; `CMap::guardingCreaturePosition`, `isCoastalTile` | S: inspected loops use bounded neighborhoods. D: adapt range and adjacency while preserving fog information and normal guard/shore rules. Full object and spell behavior needs further source analysis. | Review effects requiring topology-aware distance or neighborhoods. |
| Is a route and minimap understandable? | `movement_and_route`, `minimap` (D) | `MapRendererPath::selectImage`; `CMinimap::showAll`, `onMapViewMoved` | S: path display uses ordinary adjacency; minimap draws a single rectangle. D: presentation must match the actual route and wrap-aware visible region. | Choose minimap representation, including view larger than one map period. |
| Can the game be resumed or played together? | `persistence_and_network`, `bounded_compatibility` (D) | `CMapHeader`, JSON map serialization; `CGameHandler::moveHero` | S: dimensions/layers and server validation exist; no toroidal field was found in inspected JSON header. D: bind topology to session identity, saves and peers; old content defaults to bounded. Version negotiation and file representation are not designed. | Choose map/save format, feature negotiation, and explicit incompatibility behavior. |
| Can maps and AI actually use the feature? | `authoring`, `ai` (D) | `MapFormatJson`; selected `Nullkiller2` exploration distance checks | S: some AI decisions use ordinary distance. Generator and all AI paths were not exhaustively analyzed. D: support must be demonstrated per profile. | Decide supported AI, editor and random generator scope for each milestone. |

No package requirement is evidence that current VCMI meets it. An empty
`implementationBindings` list is intentional: existing files are baseline
evidence, not implementations of the future toroidal contract.

## Scenarios for implementation and acceptance

These are proposed behavior checks, not executed tests. Unless specified,
use a `100 × 100` level, a revealed clear destination, sufficient movement
points, and otherwise legal movement. In each direction verify both forward
and reverse traversal, and compare movement cost with the corresponding
non-seam step over equivalent terrain.

| ID | Given / action | Required observable outcome |
| --- | --- | --- |
| G1 | Walk west from `(0,45)` and east from `(99,45)` | Arrive at `(99,45)` and `(0,45)` respectively, same level. |
| G2 | Walk north from `(28,0)` and south from `(28,99)` | Arrive at `(28,99)` and `(28,0)` respectively, same level. |
| G3 | Walk northwest from `(0,0)` | Arrive at `(99,99)`; test the other three corner diagonals and their reverse steps as well. |
| G4 | Cross either axis diagonally away from a corner | The coordinate on that axis wraps and the other changes by one, with normal diagonal rules. |
| G5 | A wrapped destination is blocked or movement points are insufficient | Server rejects the move; hero and movement points remain as normal failed movement requires. |
| G6 | A route target is reachable by a shorter legal route across a seam | A displayed/executed route can use the seam subject to normal knowledge, cost and route-selection rules; no illegal traversal is introduced. |
| G7 | Put a guard or vision target just across an edge or corner | Guard interaction and fog reveal match an equivalent non-seam adjacency/radius, including hidden-information rules. |
| G8 | Cross a seam by boat, embark or disembark there | Apply the normal coast, vehicle and diagonal constraints, including blocked transitions. |
| G9 | Cross each type of seam on surface and underground, then use an existing inter-level passage | Each crossing stays on its original level; the passage changes levels according to its ordinary rules. |
| G10 | Cross equivalent seam and non-seam steps over matching terrain and roads, including a visitable object and limited movement points | Normal costs, visit direction, object events and movement-point effects agree; blocked or illegal entry is not applied. |
| G11 | Place revealed and hidden cells and a guard across a seam and at a corner | Sight and guard interaction follow ordinary rules without revealing hidden information outside the legitimate sight range. |
| V1 | Scroll slowly and continuously across each edge and corner | The viewport shows adjacent opposite-side content with no border strip or discontinuous jump; paths, objects and fog align. |
| V2 | Hover and left/right-click an object displayed across each seam | The same canonical tile/object is selected or described as at its ordinary location, subject to visibility. |
| V3 | Animate a hero crossing an edge and a corner | The hero takes one adjacent step in the correct direction; camera and route indicators stay continuous. |
| V4 | Zoom out until the viewport spans a full map period or more | All visible copies render and accept input consistently; no duplicate game-state objects are created. |
| V5 | View crosses one seam, then both | Minimap indicates all visible portions of the map and maps selection back correctly. |
| P1 | Save and reload after a seam crossing | Topology, hero position, fog and other relevant game state are preserved. |
| P2 | Open a legacy map or save with no topology declaration | Original bounded behavior remains; no implicit wrapping. |
| P3 | Multiplayer peers perform a legal and an illegal seam move | Supported peers agree on authoritative result and visibility; unsupported topology is rejected or reported explicitly. |
| P4 | A supported session encounters a peer or save reader without required topology support | The session or load reports incompatibility; it must not silently interpret the map as bounded. |
| A1 | Supported AI has a target across a seam | AI can plan and execute a legal wrapped route without treating the local neighbor as distant. |
| A2 | A player selects an AI profile that does not support the topology | The profile's lack of support is identified; it is not advertised as toroidal-compatible. |
| E1 | Open/create a supported toroidal map | Seam terrain and passability can be authored coherently; if generation is advertised, generated seams meet the same topology. |
| E2 | A supported random generator produces a toroidal map | Generated terrain, passages and any advertised roads and rivers respect the joined topology. Applicability depends on a supported generator being offered. |

Analytical witness A: on a 100 × 100 level, for `(0,0)` and direction
`(-1,-1)`, independent wrapping produces `(99,99)` on the same level. This
establishes the intended arithmetic only; it is not runtime evidence.

## Open decisions and boundaries of the pilot

1. **Seam-spanning static objects:** provisional pilot restriction: a static
   object's visible, blocking or visitable cells must stay within the map.
   The editor reports violations and the JSON map writer and reader reject
   them. Empty mask cells outside the rectangle are allowed. This does not
   implement objects occupying both sides of a seam; decide separately whether
   to add that support. Hero and boat movement across seams remains required
   and is not established by the exception from this validation.
2. **Minimum map dimensions:** the initial file-format implementation requires
   width and height of at least 3. Extents of 1 or 2 make distinct directions
   lead to the same canonical cell; supporting them remains a separate decision.
3. **Topology configuration:** choose per-map or per-level representation,
   editor controls, VCMI map format and save compatibility/version negotiation.
   Existing `.h3m` compatibility cannot be assumed to carry a new flag.
   The pilot currently gates toroidal sessions using a numeric serialization
   version allocated locally after the inspected upstream baseline. That
   rejects clients older than the baseline, but a future independent upstream
   release could reuse that number for unrelated changes. Numeric ordering is
   therefore not proof that a peer or save reader understands this fork's
   topology. A distinct capability/format identity and compatibility tests
   are still required before advertising multiplayer or safe interchange.
4. **Generation and map art:** decide whether an initial release requires a
   wrap-aware random generator. Opposite cells need coherent transitions, not
   identical terrain; roads and rivers need suitable artwork and continuity.
5. **AI and platforms:** select supported AI profiles, multiplayer versions,
   rendering backends and device configurations; verify each claimed profile.
6. **Other spatial mechanics:** audit spells, movement layers, object footprints,
   audio, event scripts, distance calculations, special map views, map editor
   operations and network rollback. This pilot does not prove their coverage.

## Baseline source identities

SHA-256 of the full files inspected; paths are repository-relative. These are
source-reading anchors, not passing test artifacts.

```text
3a195e6682ab6c1a187a86fb241b7581397bc30d29705cd0a449eac4137a8816  lib/mapping/CMap.h
cf8025538c58b2eeec83ce53bced719019f9829cb5805dbc8e98d2b0976f7318  lib/mapping/CMap.cpp
ea1cd606f1e129e35e1fc1396b3cb16d90462fe7feb8dcf2b10efb7ad3afda5f  lib/pathfinder/CPathfinder.cpp
44ee99ccf546be43550f818c3d8d2e8c92b82813f24787ecadd79dbb55dda3cd  server/CGameHandler.cpp
15bade43d2c0dacca9eaa9f4937482631ed6925fdc8beed21621a71b749d811f  client/mapView/MapViewController.cpp
141f8dd0d356c4e6c454fd8fbb81e6ffd42dc947ed086818d4e638846bac82e7  client/mapView/MapViewModel.cpp
7c28c04b95815c544e15cdba01d0a8930242b326db8f471f2cd7c8d04bd60637  client/mapView/MapRenderer.cpp
9cd81137285e58da5f132bfb7b68afe8476051e9cfaddb5d422f85e87bc7d3c3  client/mapView/MapViewActions.cpp
e037d53c48b98f18b658b93da66249e53cfce165e470e0629755aadaf8f082b1  client/mapView/MapRendererContextState.cpp
e7b88cf6c06c44259fda19b22b8af4c47279e2f8949691d2551a5d363373c41e  client/adventureMap/CMinimap.cpp
d63ad7ec9e595127497f43954949e992ddee9abe8055e81c6f21bf5466d57abd  lib/callback/CGameInfoCallback.cpp
af0562166bdde99ed2c060633c19e21f28d8e995bb416ff6fbc3faca7ed6ef3c  AI/Nullkiller2/Helpers/ExplorationHelper.cpp
b97a0da9c4a91bfc2f575a046feba7e8d08fe31c06021afbd042c8bcf9079ba5  lib/mapping/MapFormatJson.cpp
7b6a69cab3392fcb197e49963eff86a627fe2c2fe47cbc5d61d2075c294ab892  lib/mapping/CMapHeader.h
```

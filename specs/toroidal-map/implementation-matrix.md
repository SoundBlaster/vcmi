# Toroidal map implementation matrix

This is a local tracking companion to the draft
[BoundarySpec](../toroidal-map.spec.yaml) and its
[scenario definitions](review.md#scenarios-for-implementation-and-acceptance).
It does not change the SpecPM schema or prove implementation, acceptance, or
runtime behavior. Baseline: VCMI commit
`30e9207276f137658da5132705c05644bfe33aee`, inspected source clean before
the specification work; only the selected entrypoints in `review.md` were
examined. Since then, implementation and limited integration tests have been
added as documented below; visual and profile-wide acceptance remains open.

## How to read and update this matrix

The axes are independent. A row can have source code, but no successful test;
a test can pass for one platform while other profiles remain unverified.

- **Implementation:** `not started` means no work is claimed for this draft
  feature on the baseline; it does not assert that all unrelated existing
  VCMI behavior is absent. Use `partial` with exact scope and code links,
  `implemented` with the covered scope and revision, or `unknown` where the
  relevant implementation has not been assessed.
- **Verification:** `not run`, `passed`, `failed`, `partial`, or `stale`.
  A result applies only to its recorded scenario, input, revision, build,
  platform and environment. Describe what remains unverified for `partial`.
- **Acceptance:** `pending`, `accepted`, `revised`, or `deferred`, with an
  identified responsible decision and scope. Confirmation of the overall
  product goal does not accept every derived detail or implementation.
- **Evidence:** link the actual source location and commit or dirty-tree
  identity, plus the test command and result artifact. For runtime checks
  record the build identity, platform, map and observed result. For visual
  continuity record a real playthrough or frame sequence; a single screenshot
  does not prove continuous scrolling. Report local, CI, multiplayer and
  installed-device results separately when relevant.

Each row is a separately trackable part of an existing requirement. A scenario
mapping is a planned check, not a claim of complete test coverage. If a row
cannot be demonstrated by its mapped scenarios, add specific scenarios before
marking it verified. Do not change a requirement to match code that fails it.
If a requirement changes, preserve traceability from its previous ID; if a
scenario, relevant implementation, build or environment changes, reassess
and mark affected results `stale` until rerun. Update the matrix in the same
change as each implementation slice and add concrete SpecPM evidence or
implementation bindings only for supported claims.

All rows below start with **Implementation: not started**, **Verification:
not run**, **Acceptance: pending**, and **Evidence: none for the target
behavior**, unless overridden in the row. The statement is scoped to the
baseline and this feature, not every possible branch or uninspected code.
Existing bounded behavior is a compatibility baseline, not proof that the
future opt-in design preserves it.

## Gameplay and topology

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `topology` | Horizontal and vertical steps wrap in both directions. | G1, G2 | partial: helper, pathfinder and server | partial: RoE and SoD single-level integration | pending | [integration record](evidence/topology-demo-integration-2026-11-09.md); other profiles unverified |
| `topology` | Diagonal crossing wraps one or both axes, including all four corners. | G3, G4 | partial: helper, pathfinder and server | partial: RoE and SoD single-seam and corner integration | pending | [integration record](evidence/topology-demo-integration-2026-11-09.md); other terrain and movement rules unverified |
| `topology` | Canonical tile coordinates and supported dimensions remain valid. | G1–G4; small-map cases to specify | partial: helper and format minimum | partial: helper, header minimum and one 3×3 corner move | pending | [unit record](evidence/topology-unit-2026-11-09.md), [format record](evidence/topology-format-2026-11-09.md); broader small-map gameplay unverified |
| `level_isolation` | Seams preserve level; normal inter-level passages continue to work. | G9 | partial: topology preserves level; gates and teleports resolve wrapped exit tiles | partial: underground movement, fog isolation, bidirectional synthetic gate traversal and one-way monolith | pending | [level record](evidence/topology-level-isolation-2026-11-09.md); [gate traversal](evidence/topology-gate-pair-2026-11-09.md); other passages and GUI unverified |
| `movement_and_route` | Crossing uses normal passability, diagonal rules, cost, movement points and visits. | G4, G5, G10 | partial: server and direction handling | partial: RoE terrain cost, cobblestone seam cost and blocked steps | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); other road, object and terrain cases remain |
| `movement_and_route` | Route finding considers legal wrapped neighbors under normal knowledge rules. | G6 | partial: neighbor enumeration | partial: direct RoE paths | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); obstacles and knowledge rules unverified |
| `movement_and_route` | Boat travel, embarking and disembarking follow normal rules at seams. | G8 | partial: wrapped pathfinder and server movement; server rejects coast-cutting on toroidal maps | partial: horizontal and corner embark/disembark, sailing across all edges and corners, one blocked corner | pending | [boat record](evidence/topology-boat-movement-2026-11-09.md); other shore configurations and animation unverified |
| `authority` | Client route and server agree on legal wrapped steps. | G1–G4, G6, P3 | partial: pathfinder and server | partial: RoE direct steps | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); multiplayer unverified |
| `authority` | Blocked or illegal crossing is rejected without applying movement effects. | G5, G10, P3 | partial: shared pathfinder/server terrain and occupancy checks, entrance, ordinary source exit and movement capabilities | partial: blocked tile, flying hero on a synthetic blocker, forbidden entrance/exit, unavailable layers, sailing-to-air/water transitions and one diagonal sailing restriction | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md), [object record](evidence/topology-resource-visit-2026-11-09.md), [boat record](evidence/topology-boat-movement-2026-11-09.md); other rejection cases unverified |
| `vision_and_guards` | Sight, fog and adjacency guards work across edges and corners without leaking hidden information. | G7, G11 | partial: range, Visions bonus, guard lookups and layer-specific transit | partial: hidden range, guard positions and removal, ground/air transit, guarded-water rejection and battle starts across an edge and a corner | pending | [guard battle and removal](evidence/topology-guard-battle-2026-11-09.md), [Visions range](evidence/topology-visions-spell-2026-11-09.md); complete guard and fog behavior unverified |
| `vision_and_guards` | Object interaction across seams follows normal rules. | G10; additional object cases to specify | partial: server movement and visit logic | partial: one resource across a corner | pending | [resource record](evidence/topology-resource-visit-2026-11-09.md); object audit incomplete |
| `vision_and_guards` | Nearest-town spell effects use toroidal distance. | spell case to specify | partial: shared nearest-town helper | partial: unskilled Town Portal and direct Reinforcements effect, bounded and wrapped, including one cross-level Town Portal | pending | [nearest-town record](evidence/topology-nearest-town-2026-11-09.md); Reinforcements full cast, transfers and GUI casting unverified |
| `vision_and_guards` | Ranged adventure spell targeting uses wrapped displacement. | spell cases to specify | partial: common ranged check and Nullkiller2 Dimension Door candidate enumeration | partial: range check and one AI landing plan | pending | [ranged spell record](evidence/topology-ranged-spells-2026-11-09.md); live casts and spell-specific cases unverified |
| `vision_and_guards` | Summon Boat selects and places a boat using topology-aware distance and adjacent water. | spell cases to specify | partial: nearest boat and boat-generator placement | partial: two server-side casts and one shipyard build on synthetic maps | pending | [Summon Boat record](evidence/topology-summon-boat-2026-11-09.md); GUI, level and tie cases unverified |

## Presentation and interaction

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `rendering` | Continuous terrain, roads, rivers, objects, overlays and fog at every edge and corner. | V1 | partial: viewport and renderer mapping; static seam-spanning objects prohibited in the pilot | partial: camera and cache-band geometry tests and spectator/human-player edge observations | pending | [viewport geometry](evidence/topology-viewport-2026-11-09.md); [player GUI record](evidence/topology-gui-player-2026-11-09.md); [corner camera samples](evidence/topology-gui-corner-2026-11-09.md); seam art and intermediate frames need testing |
| `rendering` | View spanning multiple map periods renders correct copies without erroneous duplicates. | V4 | partial: repeated tile mapping | partial: one zoomed-out view with several copies | pending | [wide zoom record](evidence/topology-gui-wide-zoom-2026-11-10.md); other terrain, overlays and animation unverified |
| `viewport_input` | Hover and clicks on visible copies address the same canonical location. | V2, V4 | partial: coordinate mapping | partial: one mouse-selected destination traverses both seams; corresponding horizontal copies activate the same route | pending | [mouse route](evidence/topology-gui-mouse-route-2026-11-09.md), [wide zoom](evidence/topology-gui-wide-zoom-2026-11-10.md); hover and other copies unverified |
| `viewport_input` | Destination selection remains correct across both seams and at wide zoom. | V2, V4; destination-specific case to specify | partial: coordinate mapping | partial: one mouse-selected route across both seams and one repeated-copy selection | pending | [mouse route](evidence/topology-gui-mouse-route-2026-11-09.md), [wide zoom](evidence/topology-gui-wide-zoom-2026-11-10.md); other destinations and scales unverified |
| `viewport_input` | Directional hero shortcuts select the adjacent tile across a seam. | directional-input case to specify | partial: wrapped destination before path selection | partial: human crossed both axes and one corner in both directions | pending | [directional input record](evidence/topology-directional-input-2026-11-09.md); [horizontal session](evidence/topology-gui-directional-2026-11-09.md); [corner session](evidence/topology-gui-corner-2026-11-09.md); remaining corners unverified |
| `movement_presentation` | Hero and boat cross as a short continuous step with correct direction and path indicator. | V3; boat-animation case to specify | partial: camera and sprite offsets | partial: game-state direction and sampled sprite/camera geometry across all seam directions; hero observed before and after a horizontal crossing | pending | [sprite geometry](evidence/topology-viewport-2026-11-09.md); [direction check](evidence/topology-demo-integration-2026-11-09.md); [game session](evidence/topology-gui-directional-2026-11-09.md); actual intermediate frames and boat unverified |
| `minimap` | Visible area and map selection remain correct across one or both seams. | V5 | partial: split radar and toroidal pixel-to-tile selection | partial: interval arithmetic, pixel mapping, spectator and human-player displays | pending | [minimap record](evidence/topology-minimap-2026-11-09.md), [player GUI record](evidence/topology-gui-player-2026-11-09.md); GUI selection and full boundary coverage unverified |
| `minimap` | Object ownership updates use the correct cells across seams. | object-update case to specify | partial: owner-change refresh and editor minimap use wrapped footprint cells | partial: synthetic corner footprint mapping | pending | [object tile record](evidence/topology-object-tile-queries-2026-11-10.md); GUI update unverified |

## Persistence, compatibility and supported profiles

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `persistence_and_network` | Save/reload preserves topology and authoritative state. | P1 | partial: versioned header serialization | partial: file round trip and GUI save/reload followed by corner crossing | pending | [save record](evidence/topology-seam-save-2026-11-09.md), [demo GUI](evidence/topology-adapted-demo-2026-11-10.md); full state and multiplayer unverified |
| `persistence_and_network` | Supported multiplayer peers agree on movement and visibility. | P3 | not started | not run | pending | none |
| `persistence_and_network` | Unsupported peers or readers cannot silently interpret topology incorrectly. | P4 | partial: versioned format, invalid-topology rejection and experimental tagged lobby capability | partial: format, packet, server-visitor, game preparation, internal-transport and TCP loopback tests; raw mismatched layout remains a disabled defense-in-depth regression | pending | [format record](evidence/topology-format-2026-11-09.md), [capability design](network-capability-design.md); full network game, old binaries and local map/save version collisions remain unverified |
| `bounded_compatibility` | Existing maps and saves remain bounded; toroidal behavior is opt-in. | P2 | partial: versioned default | partial: legacy header, bounded save and H3M-to-VMAP conversion | pending | [legacy editor record](evidence/topology-legacy-editor-2026-11-10.md); full legacy gameplay regression unverified |
| `legacy_footprint_wrapping` | Converted legacy object cells wrap across edges and corners without being discarded on empty terrain or obstacle overlap. | E4–E6 | partial: in-memory occupancy, drawing-order, wind area and selected interactions; static authoring rejected | partial: synthetic footprint, blocker, wind and terrain tests; real legacy boundary audit | pending | [object record](evidence/topology-object-tile-queries-2026-11-10.md), [wind initialization](evidence/topology-boat-movement-2026-11-09.md), [legacy diagnostic](evidence/topology-legacy-editor-2026-11-10.md); serialization, real overlap and visual behavior unverified |
| `authoring` | A supported authored map can have coherent passable seams. | E1 | partial: editor topology setting, wrapped terrain and line brushes, adapted demo fixture; static-footprint restriction | partial: terrain corner and line turns, topology switch, editor save/reopen and demo GUI traversal | pending | [adapted demo](evidence/topology-adapted-demo-2026-11-10.md), [legacy editor record](evidence/topology-legacy-editor-2026-11-10.md); existing-map seam repair unverified |
| `authoring` | Any supported random generator respects topology and template connections. | E2, E3 | not started | not run | pending | [generator design candidate](template-generator-design.md); generator and individual template support are conditional |
| `ai` | Each advertised supported AI can plan and act using legal wrapped routes. | A1 | partial: Nullkiller2 neighborhoods, exploration heuristics, portal planning and destination-object precheck | partial: scripted routes and interaction precheck, nearest-town planning, and real Nullkiller2 actions across both seams in one game | pending | [AI route record](evidence/topology-ai-direct-route-2026-11-09.md); [Town Portal planning](evidence/topology-ai-town-portal-2026-11-09.md); [game session](evidence/topology-gui-directional-2026-11-09.md); full AI audit and profile support undecided |
| `ai` | Unsupported AI profiles are identified as unsupported. | A2 | partial: editor warns AI is unverified | not run | pending | [editor status record](evidence/topology-editor-status-2026-11-09.md); no profile-specific selection warning |

## Specification evidence discipline

`evidence_not_implementation` is a documentation/provenance constraint, not
a gameplay capability. At this baseline the draft explicitly states its limits
in `review.md` and `validation.md`; it has no runtime scenario. Its editorial
review remains pending. Future source links and successful executions must be
scoped to exactly what they demonstrate; their presence must not be used as a
blanket conformance claim.

The [coordinate audit](coordinate-audit.md) and its [candidate index](coordinate-audit-index.md)
track the ongoing source review. Search matches are not automatically defects,
and the search does not prove that every relevant calculation has been found.

## Execution record

The isolated helper tests are recorded in
[`evidence/topology-unit-2026-11-09.md`](evidence/topology-unit-2026-11-09.md).
The shared pixel wrapping and its arithmetic tests are recorded in
[`evidence/topology-pixel-wrap-2026-11-09.md`](evidence/topology-pixel-wrap-2026-11-09.md).
The asset-independent CTest target is recorded in
[`evidence/topology-ctest-2026-11-09.md`](evidence/topology-ctest-2026-11-09.md).
The format and save test attempt is recorded in
[`evidence/topology-format-2026-11-09.md`](evidence/topology-format-2026-11-09.md).
The gameplay movement test attempt is recorded in
[`evidence/topology-movement-2026-11-09.md`](evidence/topology-movement-2026-11-09.md).
The client build is recorded in
[`evidence/topology-viewport-2026-11-09.md`](evidence/topology-viewport-2026-11-09.md).
The map-editor build is recorded in
[`evidence/topology-editor-2026-11-09.md`](evidence/topology-editor-2026-11-09.md).
The AI and combat-direction build is recorded in
[`evidence/topology-ai-2026-11-09.md`](evidence/topology-ai-2026-11-09.md).
The static seam-object restriction and its unexecuted integration test are recorded in
[`evidence/topology-seam-validation-2026-11-09.md`](evidence/topology-seam-validation-2026-11-09.md).
The one-way portal AI changes are recorded in
[`evidence/topology-portal-ai-2026-11-09.md`](evidence/topology-portal-ai-2026-11-09.md).
The isolated sight-range enumeration checks are recorded in
[`evidence/topology-range-2026-11-09.md`](evidence/topology-range-2026-11-09.md).
The partial ambient-audio adjustment is recorded in
[`evidence/topology-audio-2026-11-09.md`](evidence/topology-audio-2026-11-09.md).
The minimap interval checks are recorded in
[`evidence/topology-minimap-2026-11-09.md`](evidence/topology-minimap-2026-11-09.md).
The editor's experimental status is recorded in
[`evidence/topology-editor-status-2026-11-09.md`](evidence/topology-editor-status-2026-11-09.md).
The limited demo integration runs are recorded in
[`evidence/topology-demo-integration-2026-11-09.md`](evidence/topology-demo-integration-2026-11-09.md).
The spectator viewport observation is recorded in
[`evidence/topology-gui-spectator-2026-11-09.md`](evidence/topology-gui-spectator-2026-11-09.md).
The later human-player viewport observation is recorded in
[`evidence/topology-gui-player-2026-11-09.md`](evidence/topology-gui-player-2026-11-09.md).
The directional hero input change is recorded in
[`evidence/topology-directional-input-2026-11-09.md`](evidence/topology-directional-input-2026-11-09.md).
The Visions spell range check is recorded in
[`evidence/topology-visions-spell-2026-11-09.md`](evidence/topology-visions-spell-2026-11-09.md).
The nearest-town spell check is recorded in
[`evidence/topology-nearest-town-2026-11-09.md`](evidence/topology-nearest-town-2026-11-09.md).
The wrapped terrain-brush checks are recorded in
[`evidence/topology-terrain-brush-2026-11-09.md`](evidence/topology-terrain-brush-2026-11-09.md).
The road and river brush checks are recorded in
[`evidence/topology-line-brush-2026-11-09.md`](evidence/topology-line-brush-2026-11-09.md).
Object occupancy groundwork is recorded in
[`evidence/topology-object-occupancy-2026-11-09.md`](evidence/topology-object-occupancy-2026-11-09.md).
Object draw-order groundwork is recorded in
[`evidence/topology-object-order-2026-11-09.md`](evidence/topology-object-order-2026-11-09.md).
Client object stamping groundwork is recorded in
[`evidence/topology-client-stamping-2026-11-09.md`](evidence/topology-client-stamping-2026-11-09.md).
Editor object stamping groundwork is recorded in
[`evidence/topology-editor-stamping-2026-11-09.md`](evidence/topology-editor-stamping-2026-11-09.md).
Wrapped object-visibility checks are recorded in
[`evidence/topology-object-visibility-2026-11-09.md`](evidence/topology-object-visibility-2026-11-09.md).
Editor topology switching is recorded in
[`evidence/topology-editor-switch-2026-11-09.md`](evidence/topology-editor-switch-2026-11-09.md).
The save-file check after crossing is recorded in
[`evidence/topology-seam-save-2026-11-09.md`](evidence/topology-seam-save-2026-11-09.md).
The boat-movement checks are recorded in
[`evidence/topology-boat-movement-2026-11-09.md`](evidence/topology-boat-movement-2026-11-09.md).
The underground seam check is recorded in
[`evidence/topology-level-isolation-2026-11-09.md`](evidence/topology-level-isolation-2026-11-09.md).
Nullkiller2 direct seam routes are recorded in
[`evidence/topology-ai-direct-route-2026-11-09.md`](evidence/topology-ai-direct-route-2026-11-09.md).
The resource visit check is recorded in
[`evidence/topology-resource-visit-2026-11-09.md`](evidence/topology-resource-visit-2026-11-09.md).
The wrapped guard battle check is recorded in
[`evidence/topology-guard-battle-2026-11-09.md`](evidence/topology-guard-battle-2026-11-09.md).
These tests cover selected gameplay scenarios, not full acceptance. Add a dated record for each run with:
scenario ID and requirement row; source commit and dirty-tree identity if any;
build identity; platform/backend and map/fixture; command or manual steps;
result and evidence path; remaining gaps; and the responsible review decision
if acceptance is claimed. Keep previous records when evidence becomes stale.

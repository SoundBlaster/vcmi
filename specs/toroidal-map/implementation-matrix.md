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
| `level_isolation` | Seams preserve level; normal inter-level passages continue to work. | G9 | partial: topology preserves level; gates and teleports resolve wrapped exit tiles | partial: underground movement, fog isolation, bidirectional synthetic gate traversal, one-way monolith and protected/unprotected whirlpool travel to an unoccupied wrapped exit and rejection of fully occupied exits | pending | [level record](evidence/topology-level-isolation-2026-11-09.md); [gate traversal](evidence/topology-gate-pair-2026-11-09.md); other passages and GUI unverified |
| `movement_and_route` | Crossing uses normal passability, diagonal rules, cost, movement points and visits. | G4, G5, G10 | partial: server and direction handling; helper owns its options | partial: RoE terrain cost, cobblestone seam cost, blocked steps and options lifetime | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md), [options lifetime](evidence/pathfinder-options-lifetime-2026-11-10.md); other road, object and terrain cases remain |
| `movement_and_route` | Route finding considers legal wrapped neighbors under normal knowledge rules. | G6 | partial: neighbor enumeration and conservative hidden-tile handling | partial: direct RoE paths and synthetic hidden terrain/coast comparisons | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md), [hidden pathfinding](evidence/topology-hidden-pathfinding-2026-11-10.md); other obstacles and knowledge rules unverified |
| `movement_and_route` | On-foot Flight and Water Walk can switch layers where both are available. | G6, G10 | partial: pathfinder AIR/WATER transitions and conditional landing validation | partial: pathfinding at all edges and corners; two executed seam transitions; TCP loopback replica planning and one executed switch | pending | [transient switches](evidence/topology-transient-switches-2026-11-10.md); other effects, vehicles and GUI unverified |
| `movement_and_route` | Boat travel, embarking and disembarking follow normal rules at seams. | G8 | partial: wrapped pathfinder and server movement; server rejects coast-cutting and sailing disembarkation onto visitable cells on toroidal maps | partial: horizontal and corner embark/disembark, sailing across all edges and corners, one blocked corner and rejected disembarkation onto a nonblocking event; GUI embark, sailing in four directions and disembark on `afb8044`, plus diagonal sailing across one seam in both directions; open-water diagonal crossings of both seams in both directions on `3478269`, `c4aa1ed` and `053a830` | pending | [boat record](evidence/topology-boat-movement-2026-11-09.md), [GUI boat fixture](evidence/topology-boat-gui-2026-11-10.md), [current launch and crossing](evidence/topology-game-launch-review-2026-11-14.md); other shore configurations and animation unverified |
| `authority` | Client route and server agree on legal wrapped steps. | G1–G4, G6, P3 | partial: pathfinder and server | partial: RoE direct steps | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); multiplayer unverified |
| `authority` | Blocked or illegal crossing is rejected without applying movement effects. | G5, G10, P3 | partial: terrain and occupancy checks, direction, movement capabilities and replicated server-confirmed source layers; older saves retain unknown-layer fallback | partial: blocked tiles, forbidden exits on land, in boats and after airborne transit, local and TCP replica replanning and one rollback; unavailable layers and sailing transitions | pending | [server source layer](evidence/topology-server-movement-layer-2026-11-10.md), [replicated layer](evidence/topology-replicated-movement-layer-2026-11-10.md), [object record](evidence/topology-resource-visit-2026-11-09.md), [boat record](evidence/topology-boat-movement-2026-11-09.md); GUI, turn boundaries, legacy unknown layers and other rejection cases unverified |
| `authority` | A turn cannot end during AIR or WATER transit; a stranded hero remains recoverable. | G12 | partial: turn guard, last-points rejection, conditional known-landing check, timer-driven ordinary landing on a clear known path and in-place landing on clear known land for timer or manual turn end | partial: TCP loopback Flight rejection, layer switch, directly driven timer landing, manual in-place landing and turn transfer on two TCP replicas; atomic multi-hero landing check; local AIR/WATER landing preservation, unknown terrain, already-stranded state, combined abilities and timer fallback | pending | [transient turn](evidence/topology-transient-turn-2026-11-10.md), [known landing](evidence/topology-known-landing-2026-11-10.md), [timer landing](evidence/topology-timer-landing-2026-11-10.md), [in-place landing](evidence/topology-in-place-landing-2026-11-10.md), [manual in-place landing](evidence/topology-manual-in-place-landing-2026-11-10.md), [timer after real movement](evidence/topology-timer-live-order-2026-11-14.md); general recovery, wall-clock scheduling, remote-network play and graphical play unverified |
| `vision_and_guards` | Sight, fog and adjacency guards work across edges and corners without leaking hidden information. | G7, G11 | partial: range, unrestricted object visibility, Visions bonus, known-guard callbacks and route planning, Dimension Door preview and AI continuation, Scuttle Boat target preview, and layer-specific transit | partial: server corner sight and serialized move on two replicas with team/level isolation, object visibility, hidden range, synthetic hidden/visible guard callbacks, routes and cursor, hidden spell targets on bounded and toroidal maps, guard positions and removal, ground/air transit, guarded-water rejection and battle starts across an edge and a corner | pending | [team and level isolation](evidence/topology-level-isolation-2026-11-09.md), [unrestricted object visibility](evidence/topology-unrestricted-object-visibility-2026-11-10.md), [hidden guards](evidence/topology-hidden-pathfinding-2026-11-10.md), [spell preview review](evidence/topology-spell-fog-review-2026-11-14.md), [hidden boat target](evidence/topology-hidden-scuttle-target-2026-11-10.md), [hidden Dimension Door terrain](evidence/topology-hidden-dimension-door-2026-11-10.md), [guard battle and removal](evidence/topology-guard-battle-2026-11-09.md), [Visions range](evidence/topology-visions-spell-2026-11-09.md); other unchecked queries, spell validation, real clients, confidentiality and rendered fog unverified |
| `vision_and_guards` | Object interaction across seams follows normal rules. | G10; additional object cases to specify | partial: server movement and visit logic | partial: resource across a corner, neutral unguarded mine capture across an edge and same-tile mine revisit after crossing | pending | [interaction record](evidence/topology-resource-visit-2026-11-09.md); object audit incomplete |
| `vision_and_guards` | Grail placement and digging remain valid at seams. | Grail-specific case to specify | partial: wrapped placement radius, hole lookup and server digging | partial: placement at one edge, wrapped hole exclusion, corner crossing and successful digging with in-memory state restoration | pending | [Grail record](evidence/topology-grail-placement-2026-11-09.md); puzzle rendering, GUI, disk save and other levels unverified |
| `vision_and_guards` | Nearest-town spell effects use toroidal distance. | spell case to specify | partial: shared nearest-town helper | partial: unskilled Town Portal and direct Reinforcements effect, bounded and wrapped, including one cross-level Town Portal | pending | [nearest-town record](evidence/topology-nearest-town-2026-11-09.md); Reinforcements full cast, transfers and GUI casting unverified |
| `vision_and_guards` | Ranged adventure spell targeting uses wrapped displacement. | spell cases to specify | partial: common ranged check and Nullkiller2 Dimension Door candidate enumeration | partial: range check, one AI landing plan, server Dimension Door edge/corner casts and Scuttle Boat across an edge | pending | [ranged spell record](evidence/topology-ranged-spells-2026-11-09.md); GUI casts and other spell-specific cases unverified |
| `vision_and_guards` | Summon Boat selects and places a boat using topology-aware distance and adjacent water. | spell cases to specify | partial: nearest boat and boat-generator placement | partial: two server-side casts and one shipyard build on synthetic maps | pending | [Summon Boat record](evidence/topology-summon-boat-2026-11-09.md); GUI, level and tie cases unverified |

## Presentation and interaction

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `rendering` | Continuous terrain, roads, rivers, objects, overlays and fog at every edge and corner. | V1 | partial: viewport, renderer and overlay coordinate mapping; static seam-spanning footprints fitting one map period can be saved | partial: geometry, overlay coordinate and object-mask checks, player edge observations, sampled road/river/terrain seams, one wrapped town sprite, a converted demo map, and wrapped-monster battle position matching | pending | [viewport geometry](evidence/topology-viewport-2026-11-09.md); [overlay copies](evidence/topology-overlay-copies-2026-11-10.md); [visual seam fixture](evidence/topology-visual-seams-2026-11-10.md); [wrapped town](evidence/topology-wrapped-footprint-vmap-2026-11-10.md); [monster battle position](evidence/topology-wrapped-monster-battle-2026-11-10.md); other overlay types, seam art and continuous frames need testing |
| `rendering` | View spanning multiple map periods renders correct copies without erroneous duplicates. | V4 | partial: repeated tile and overlay mapping | partial: zoomed-out world view shows matching hero, town and resource icons on repeated copies; overlay coordinate and object-mask tests | pending | [wide zoom record](evidence/topology-gui-wide-zoom-2026-11-10.md), [overlay copies and before/after GUI](evidence/topology-overlay-copies-2026-11-10.md); other terrain, overlays, zoom levels and animation unverified |
| `viewport_input` | Independent camera scrolling crosses seams and remains responsive at low speeds. | V1; manual-scrolling case to specify | partial: wrapped camera and fractional keyboard/border scroll accumulation | partial: keyboard endpoints across both axes at normal and reduced speeds, accumulation tests, and user-recorded camera movement across both axes | pending | [manual scrolling record](evidence/topology-adapted-demo-2026-11-10.md), [user recording](evidence/topology-user-screen-recording-2026-11-10.md); border input, frame pacing and other surfaces unverified |
| `viewport_input` | Hover and clicks on visible copies address the same canonical location. | V2, V4 | partial: coordinate mapping | partial: one mouse-selected destination traverses both seams; corresponding horizontal copies activate the same route | pending | [mouse route](evidence/topology-gui-mouse-route-2026-11-09.md), [wide zoom](evidence/topology-gui-wide-zoom-2026-11-10.md); hover and other copies unverified |
| `viewport_input` | Destination selection remains correct across both seams and at wide zoom. | V2, V4; destination-specific case to specify | partial: coordinate mapping | partial: one mouse-selected route across both seams and one repeated-copy selection | pending | [mouse route](evidence/topology-gui-mouse-route-2026-11-09.md), [wide zoom](evidence/topology-gui-wide-zoom-2026-11-10.md); other destinations and scales unverified |
| `viewport_input` | Directional hero shortcuts select the adjacent tile across a seam. | directional-input case to specify | partial: wrapped destination before path selection | partial: human crossed both axes and one corner in both directions, including a later run on build `4255569` | pending | [directional input record](evidence/topology-directional-input-2026-11-09.md); [horizontal session](evidence/topology-gui-directional-2026-11-09.md); [corner session](evidence/topology-gui-corner-2026-11-09.md); [current build](evidence/topology-adapted-demo-2026-11-10.md); remaining corners unverified |
| `movement_presentation` | Hero and boat cross as a short continuous step with correct direction and path indicator. | V3; boat-animation case to specify | partial: camera, sprite offsets and periodic image fragments | partial: game-state direction, sampled geometry and software compositing; slow hero/camera intermediate frames at an edge and corner; normal-speed hero video samples in four cardinal directions and one corner, plus sampled boat video in four directions | pending | [sprite geometry](evidence/topology-viewport-2026-11-09.md); [slow GUI samples](evidence/topology-adapted-demo-2026-11-10.md); [hero seam video](evidence/topology-hero-seam-video-2026-11-10.md), [boat GUI](evidence/topology-boat-gui-2026-11-10.md), [current corner samples](evidence/topology-boat-corner-current-2026-11-10.md); uninterrupted frames and diagonal boat animation, including open-water corner crossings, unverified |
| `minimap` | Visible area and map selection remain correct across one or both seams. | V5 | partial: split radar and toroidal pixel-to-tile selection | partial: interval arithmetic, pixel mapping, spectator and human-player displays | pending | [minimap record](evidence/topology-minimap-2026-11-09.md), [player GUI record](evidence/topology-gui-player-2026-11-09.md); GUI selection and full boundary coverage unverified |
| `minimap` | Object ownership updates use the correct cells across seams. | object-update case to specify | partial: owner-change refresh and editor minimap use wrapped footprint cells | partial: synthetic corner footprint mapping | pending | [object tile record](evidence/topology-object-tile-queries-2026-11-10.md); GUI update unverified |

## Persistence, compatibility and supported profiles

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `persistence_and_network` | Save/reload preserves topology and authoritative state. | P1 | partial: versioned header, replicated movement-layer and server serialization, experimental save marker | partial: server save/load, airborne layer round trip and prior-server-state migration, clipped occupancy round trip, boat identity and corner sailing, and prior GUI save/reload | pending | [replicated layer](evidence/topology-replicated-movement-layer-2026-11-10.md), [clipped occupancy save](evidence/topology-clipped-save-2026-11-10.md), [boat save](evidence/topology-boat-movement-2026-11-09.md), [save marker](evidence/topology-save-marker-2026-11-10.md), [demo GUI](evidence/topology-adapted-demo-2026-11-10.md); full state and multiplayer unverified |
| `persistence_and_network` | Supported multiplayer peers agree on movement and visibility. | P3 | partial: capability negotiation and server broadcast | partial: two TCP listeners negotiate and start from a VMAP, observe corner resource visit, turn change, legal seam moves and a rejected seam move, with position, movement points and surface fog agreement | pending | [TCP movement and lobby record](evidence/topology-tcp-movement-2026-11-10.md); graphical clients, remote hosts, broader movement and confidentiality unverified |
| `persistence_and_network` | Unsupported peers or readers cannot silently interpret topology incorrectly. | P4 | partial: tagged new toroidal VMAPs and saves, experimental tagged lobby capability | partial: VMAP round trips, save, packet, game preparation, internal-transport and TCP loopback tests, including rejected mixed-capability lobby start and successful retry; raw mismatched packet layout remains a disabled defense-in-depth regression | pending | [VMAP marker](evidence/topology-format-2026-11-09.md), [save marker](evidence/topology-save-marker-2026-11-10.md), [capability design](network-capability-design.md), [TCP lobby](evidence/topology-tcp-movement-2026-11-10.md); untagged prototypes, old binaries and graphical multiplayer remain unverified |
| `bounded_compatibility` | Existing maps and saves remain bounded; toroidal behavior is opt-in. | P2 | partial: versioned default | partial: legacy header, bounded save and H3M-to-VMAP conversion | pending | [legacy editor record](evidence/topology-legacy-editor-2026-11-10.md); full legacy gameplay regression unverified |
| `legacy_footprint_wrapping` | Converted legacy object cells wrap across edges and corners without being discarded on empty terrain or obstacle overlap. | E4–E6 | partial: wrapped occupancy, selected interactions, authoring rebuild and VMAP serialization; oversized footprints rejected | partial: synthetic and two real-map two-level round trips, loaded resource interaction, one real town sprite observed, town appearance lookup, generated heroes at wrapped towns and converted demo started; default rebuild retains explicitly shown fully clipped occupancy during topology changes and reindex; new tagged saves preserve it, while older saves cannot reconstruct missing state | pending | [clipped occupancy rebuild](evidence/topology-clipped-rebuild-2026-11-10.md), [clipped occupancy save](evidence/topology-clipped-save-2026-11-10.md), [wrapped VMAP and visual check](evidence/topology-wrapped-footprint-vmap-2026-11-10.md), [town anchor lookup](evidence/topology-town-anchor-terrain-2026-11-10.md), [main town startup](evidence/topology-main-town-start-2026-11-10.md), [real legacy round trips and launch](evidence/topology-legacy-editor-2026-11-10.md); interactive-overlap policy and general compatibility unverified |
| `legacy_footprint_wrapping` | Position-targeted scenario objectives resolve objects moved across seams. | objective cases to specify | partial: wrapped objective lookup and editor move/undo tracking | partial: five town-targeted condition types, selected hero targets, VMAP startup and one control evaluation; invalid-level and bounded checks | pending | [objective resolution and editor moves](evidence/topology-wrapped-objectives-2026-11-10.md); other win/loss execution and overlapping targets unverified |
| `authoring` | A supported authored map can have coherent passable seams. | E1 | partial: topology setting, wrapped brushes, terrain redraw, object selection/preview geometry and main-town/objective reference updates, adapted demo, seam-interaction warning and unstable-visit-order save guard | partial: terrain corner and line turns, road-source and redraw geometry, topology switch, editor save/reopen, town and objective move/undo with game startup, demo GUI traversal and writer guard; new diagnostics and object preview UI unverified | pending | [adapted demo](evidence/topology-adapted-demo-2026-11-10.md), [main town startup](evidence/topology-main-town-start-2026-11-10.md), [objective moves](evidence/topology-wrapped-objectives-2026-11-10.md), [editor road redraw](evidence/topology-editor-road-redraw-2026-11-10.md), [object selection geometry](evidence/topology-editor-object-selection-2026-11-10.md), [visit-order guard](evidence/topology-wrapped-visit-order-2026-11-10.md); other scenario references and existing-map seam repair unverified |
| `authoring` | Any supported random generator respects topology and template connections. | E2, E3 | not started | not run | pending | [generator design candidate](template-generator-design.md); generator and individual template support are conditional |
| `ai` | Each advertised supported AI can plan and act using legal wrapped routes. | A1 | partial: Nullkiller2 neighborhoods, exploration heuristics, portal planning, confirmed initial layer and destination-object precheck | partial: scripted routes and interaction precheck, nearest-town planning, bounded and wrapped patrol paths, AI actor layer selection, and real Nullkiller2 actions across both seams in one game | pending | [AI route record](evidence/topology-ai-direct-route-2026-11-09.md); [replicated layer](evidence/topology-replicated-movement-layer-2026-11-10.md); [patrol radius](evidence/topology-ai-patrol-2026-11-10.md); [Town Portal planning](evidence/topology-ai-town-portal-2026-11-09.md); [game session](evidence/topology-gui-directional-2026-11-09.md); full AI audit and profile support undecided |
| `ai` | Unsupported AI profiles are identified as unsupported. | A2 | partial: editor and scenario information warn that AI support is unverified | partial: translation files checked and scenario-list warning observed in current GUI | pending | [player status record](evidence/topology-ai-status-player-2026-11-10.md), [editor status](evidence/topology-editor-status-2026-11-09.md), [demo GUI](evidence/topology-adapted-demo-2026-11-10.md); profile-specific support and other displays unverified |

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

The [current full-suite record](evidence/topology-full-suite-2026-11-10.md)
includes the three optional historical-format fixtures; four tests remain disabled.

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
The former static seam-object restriction is recorded in
[`evidence/topology-seam-validation-2026-11-09.md`](evidence/topology-seam-validation-2026-11-09.md);
its later partial removal is recorded in
[`evidence/topology-wrapped-footprint-vmap-2026-11-10.md`](evidence/topology-wrapped-footprint-vmap-2026-11-10.md).
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

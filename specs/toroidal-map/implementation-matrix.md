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
| `topology` | Diagonal crossing wraps one or both axes, including all four corners. | G3, G4 | partial: helper, pathfinder and server | partial: RoE and SoD corner integration | pending | [integration record](evidence/topology-demo-integration-2026-11-09.md); other diagonal cases unverified |
| `topology` | Canonical tile coordinates and supported dimensions remain valid. | G1–G4; small-map cases to specify | partial: helper and format minimum | partial: isolated helper tests | pending | [unit record](evidence/topology-unit-2026-11-09.md); format minimum is untested |
| `level_isolation` | Seams preserve level; normal inter-level passages continue to work. | G9 | partial: coordinate helper only | partial: isolated helper tests | pending | [unit record](evidence/topology-unit-2026-11-09.md); no passage test |
| `movement_and_route` | Crossing uses normal passability, diagonal rules, cost, movement points and visits. | G4, G5, G10 | partial: server and direction handling | partial: RoE terrain cost and one blocked step | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); object, road and other cases remain |
| `movement_and_route` | Route finding considers legal wrapped neighbors under normal knowledge rules. | G6 | partial: neighbor enumeration | partial: direct RoE paths | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); obstacles and knowledge rules unverified |
| `movement_and_route` | Boat travel, embarking and disembarking follow normal rules at seams. | G8 | not started | not run | pending | none |
| `authority` | Client route and server agree on legal wrapped steps. | G1–G4, G6, P3 | partial: pathfinder and server | partial: RoE direct steps | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); multiplayer unverified |
| `authority` | Blocked or illegal crossing is rejected without applying movement effects. | G5, G10, P3 | partial: server rules adapted | partial: one blocked RoE tile | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); other rejection cases unverified |
| `vision_and_guards` | Sight, fog and adjacency guards work across edges and corners without leaking hidden information. | G7, G11 | partial: range and guard lookups | partial: hidden range and selected guard positions | pending | [integration record](evidence/topology-demo-integration-2026-11-09.md); complete guard interactions and fog behavior unverified |
| `vision_and_guards` | Object interaction across seams follows normal rules. | G10; additional object cases to specify | not started | not run | pending | none; object audit incomplete |

## Presentation and interaction

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `rendering` | Continuous terrain, roads, rivers, objects, overlays and fog at every edge and corner. | V1 | partial: viewport and renderer mapping; static seam-spanning objects prohibited in the pilot | partial: spectator edge observations | pending | [GUI record](evidence/topology-gui-spectator-2026-11-09.md); seam art and continuous visual behavior need testing |
| `rendering` | View spanning multiple map periods renders correct copies without erroneous duplicates. | V4 | partial: repeated tile mapping | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); no visual run |
| `viewport_input` | Hover and clicks on visible copies address the same canonical location. | V2, V4 | partial: coordinate mapping | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md) |
| `viewport_input` | Destination selection remains correct across both seams and at wide zoom. | V2, V4; destination-specific case to specify | partial: coordinate mapping | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); destination case unspecified |
| `movement_presentation` | Hero and boat cross as a short continuous step with correct direction and path indicator. | V3; boat-animation case to specify | partial: camera and sprite offsets | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); runtime visual check required |
| `minimap` | Visible area and map selection remain correct across one or both seams. | V5 | partial: split radar | partial: isolated interval arithmetic and spectator display | pending | [GUI record](evidence/topology-gui-spectator-2026-11-09.md); selection and player mode unverified |

## Persistence, compatibility and supported profiles

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `persistence_and_network` | Save/reload preserves topology and authoritative state. | P1 | partial: versioned header serialization | partial: file round trip and in-memory state reload after seam crossing | pending | [integration record](evidence/topology-demo-integration-2026-11-09.md); full-file save after crossing and multiplayer unverified |
| `persistence_and_network` | Supported multiplayer peers agree on movement and visibility. | P3 | not started | not run | pending | none |
| `persistence_and_network` | Unsupported peers or readers cannot silently interpret topology incorrectly. | P4 | partial: versioned format and lobby start guard | not run | pending | [format record](evidence/topology-format-2026-11-09.md); locally assigned version can collide with future upstream, so capability identity remains unresolved |
| `bounded_compatibility` | Existing maps and saves remain bounded; toroidal behavior is opt-in. | P2 | partial: versioned default | partial: legacy header and bounded save | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md); full legacy gameplay regression unverified |
| `authoring` | A supported authored map can have coherent passable seams. | E1 | partial: editor topology setting, wrapped terrain and line brushes, static-footprint restriction | partial: terrain corner and line turns | pending | [terrain record](evidence/topology-terrain-brush-2026-11-09.md), [line record](evidence/topology-line-brush-2026-11-09.md); GUI editing and existing-map seam repair unverified |
| `authoring` | Any supported random generator respects topology. | E2 | not started | not run | pending | none; generator support is conditional |
| `ai` | Each advertised supported AI can plan and act using legal wrapped routes. | A1 | partial: Nullkiller2 neighborhoods, exploration heuristics and portal tasks | not run | pending | [AI record](evidence/topology-ai-2026-11-09.md), [portal record](evidence/topology-portal-ai-2026-11-09.md); full AI audit and profile support undecided |
| `ai` | Unsupported AI profiles are identified as unsupported. | A2 | partial: editor warns AI is unverified | not run | pending | [editor status record](evidence/topology-editor-status-2026-11-09.md); no profile-specific selection warning |

## Specification evidence discipline

`evidence_not_implementation` is a documentation/provenance constraint, not
a gameplay capability. At this baseline the draft explicitly states its limits
in `review.md` and `validation.md`; it has no runtime scenario. Its editorial
review remains pending. Future source links and successful executions must be
scoped to exactly what they demonstrate; their presence must not be used as a
blanket conformance claim.

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
The wrapped terrain-brush checks are recorded in
[`evidence/topology-terrain-brush-2026-11-09.md`](evidence/topology-terrain-brush-2026-11-09.md).
The road and river brush checks are recorded in
[`evidence/topology-line-brush-2026-11-09.md`](evidence/topology-line-brush-2026-11-09.md).
These tests cover selected gameplay scenarios, not full acceptance. Add a dated record for each run with:
scenario ID and requirement row; source commit and dirty-tree identity if any;
build identity; platform/backend and map/fixture; command or manual steps;
result and evidence path; remaining gaps; and the responsible review decision
if acceptance is claimed. Keep previous records when evidence becomes stale.

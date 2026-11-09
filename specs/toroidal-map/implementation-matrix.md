# Toroidal map implementation matrix

This is a local tracking companion to the draft
[BoundarySpec](../toroidal-map.spec.yaml) and its
[scenario definitions](review.md#scenarios-for-implementation-and-acceptance).
It does not change the SpecPM schema or prove implementation, acceptance, or
runtime behavior. Baseline: VCMI commit
`30e9207276f137658da5132705c05644bfe33aee`, inspected source clean before
the specification work; only the selected entrypoints in `review.md` were
examined. A coordinate helper has since been added and tested in isolation;
no toroidal gameplay scenario execution is claimed.

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
| `topology` | Horizontal and vertical steps wrap in both directions. | G1, G2 | partial: coordinate helper only | partial: isolated helper tests | pending | [unit record](evidence/topology-unit-2026-11-09.md); no hero movement |
| `topology` | Diagonal crossing wraps one or both axes, including all four corners. | G3, G4 | partial: coordinate helper only | partial: isolated helper tests | pending | [unit record](evidence/topology-unit-2026-11-09.md); no hero movement |
| `topology` | Canonical tile coordinates and supported dimensions remain valid. | G1–G4; small-map cases to specify | partial: helper and format minimum | partial: isolated helper tests | pending | [unit record](evidence/topology-unit-2026-11-09.md); format minimum is untested |
| `level_isolation` | Seams preserve level; normal inter-level passages continue to work. | G9 | partial: coordinate helper only | partial: isolated helper tests | pending | [unit record](evidence/topology-unit-2026-11-09.md); no passage test |
| `movement_and_route` | Crossing uses normal passability, diagonal rules, cost, movement points and visits. | G4, G5, G10 | partial: server and direction handling | not run | pending | [movement record](evidence/topology-movement-2026-11-09.md); object and cost cases remain |
| `movement_and_route` | Route finding considers legal wrapped neighbors under normal knowledge rules. | G6 | partial: neighbor enumeration | not run | pending | [movement record](evidence/topology-movement-2026-11-09.md); route selection coverage needs refinement |
| `movement_and_route` | Boat travel, embarking and disembarking follow normal rules at seams. | G8 | not started | not run | pending | none |
| `authority` | Client route and server agree on legal wrapped steps. | G1–G4, G6, P3 | partial: pathfinder and server | not run | pending | [movement record](evidence/topology-movement-2026-11-09.md); multiplayer unverified |
| `authority` | Blocked or illegal crossing is rejected without applying movement effects. | G5, G10, P3 | partial: server rules adapted | not run | pending | [movement record](evidence/topology-movement-2026-11-09.md); rejection cases unverified |
| `vision_and_guards` | Sight, fog and adjacency guards work across edges and corners without leaking hidden information. | G7, G11 | partial: range and guard lookups | not run | pending | [movement record](evidence/topology-movement-2026-11-09.md); no runtime observation |
| `vision_and_guards` | Object interaction across seams follows normal rules. | G10; additional object cases to specify | not started | not run | pending | none; object audit incomplete |

## Presentation and interaction

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `rendering` | Continuous terrain, roads, rivers, objects, overlays and fog at every edge and corner. | V1 | partial: viewport and renderer mapping | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); static seam-spanning objects need work |
| `rendering` | View spanning multiple map periods renders correct copies without erroneous duplicates. | V4 | partial: repeated tile mapping | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); no visual run |
| `viewport_input` | Hover and clicks on visible copies address the same canonical location. | V2, V4 | partial: coordinate mapping | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md) |
| `viewport_input` | Destination selection remains correct across both seams and at wide zoom. | V2, V4; destination-specific case to specify | partial: coordinate mapping | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); destination case unspecified |
| `movement_presentation` | Hero and boat cross as a short continuous step with correct direction and path indicator. | V3; boat-animation case to specify | partial: camera and sprite offsets | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); runtime visual check required |
| `minimap` | Visible area and map selection remain correct across one or both seams. | V5 | partial: split radar | not run | pending | [viewport record](evidence/topology-viewport-2026-11-09.md); wide-view case to specify |

## Persistence, compatibility and supported profiles

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `persistence_and_network` | Save/reload preserves topology and authoritative state. | P1 | partial: versioned header serialization | not run | pending | [format record](evidence/topology-format-2026-11-09.md); game reload unverified |
| `persistence_and_network` | Supported multiplayer peers agree on movement and visibility. | P3 | not started | not run | pending | none |
| `persistence_and_network` | Unsupported peers or readers cannot silently interpret topology incorrectly. | P4 | partial: versioned format and lobby start guard | not run | pending | [format record](evidence/topology-format-2026-11-09.md); behavior unverified |
| `bounded_compatibility` | Existing maps and saves remain bounded; toroidal behavior is opt-in. | P2 | partial: versioned default | not run | pending | [format record](evidence/topology-format-2026-11-09.md); no legacy regression execution |
| `authoring` | A supported authored map can have coherent passable seams. | E1 | partial: editor topology setting | not run | pending | [editor record](evidence/topology-editor-2026-11-09.md); seam validation and runtime unverified |
| `authoring` | Any supported random generator respects topology. | E2 | not started | not run | pending | none; generator support is conditional |
| `ai` | Each advertised supported AI can plan and act using legal wrapped routes. | A1 | partial: Nullkiller2 neighborhood and exploration heuristics | not run | pending | [AI record](evidence/topology-ai-2026-11-09.md); full AI audit and profile support undecided |
| `ai` | Unsupported AI profiles are identified as unsupported. | A2 | not started | not run | pending | none |

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
No gameplay scenario has been executed. Add a dated record for each run with:
scenario ID and requirement row; source commit and dirty-tree identity if any;
build identity; platform/backend and map/fixture; command or manual steps;
result and evidence path; remaining gaps; and the responsible review decision
if acceptance is claimed. Keep previous records when evidence becomes stale.

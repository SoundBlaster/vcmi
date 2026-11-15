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
| `topology` | Horizontal and vertical steps wrap in both directions. | G1, G2 | partial: helper, pathfinder and server | partial: RoE and SoD integration, plus rectangular VMAP movement on both levels | pending | [integration record](evidence/topology-demo-integration-2026-11-09.md), [rectangular maps](evidence/topology-rectangular-vmap-2026-11-15.md); other profiles unverified |
| `topology` | Diagonal crossing wraps one or both axes, including all four corners. | G3, G4 | partial: helper, pathfinder and server | partial: RoE and SoD single-seam and corner integration, rectangular VMAP movement on both levels; graphical client collected a resource and crossed one corner on square and 36×23 maps, including current build `ab35306` | pending | [integration record](evidence/topology-demo-integration-2026-11-09.md), [rectangular maps](evidence/topology-rectangular-vmap-2026-11-15.md), [current preview](evidence/topology-preview-ab35306-2026-11-15.md), [earlier clean demo corner](evidence/topology-preview-b495737-2026-11-15.md); other terrain and movement rules unverified |
| `topology` | Canonical tile coordinates and supported dimensions remain valid. | G1–G4; small-map cases to specify | partial: helper and format minimum | partial: helper, header minimum, 3×3 movement and resource visits across all edges and corners on both levels, and one guard across a corner | pending | [unit record](evidence/topology-unit-2026-11-09.md), [format record](evidence/topology-format-2026-11-09.md), [minimum-size movement](evidence/topology-minimum-size-movement-2026-11-15.md); other small-map objects and broader gameplay unverified |
| `level_isolation` | Seams preserve level; normal inter-level passages continue to work. | G9 | partial: topology preserves level; gates and teleports resolve wrapped exit tiles and revalidate selected destinations; emergency return rejects an origin on a different level | partial: underground movement, fog isolation, bidirectional synthetic gate traversal and friendly exchange, one-way monolith and protected/unprotected whirlpool travel, occupied-exit revalidation on bounded and toroidal monoliths and wrapped whirlpools, including TCP delivery to two replicas and request-connection ordering for a stale monolith exit; stale cross-level recovery origin before and after memory serialization | pending | [level record](evidence/topology-level-isolation-2026-11-09.md); [gate traversal](evidence/topology-gate-pair-2026-11-09.md); [stale exits and TCP reply order](evidence/teleport-stale-exits-2026-11-15.md); [recovery level check](evidence/topology-transient-recovery-level-2026-11-14.md); other passages, graphical client route stopping and remote peers unverified |
| `movement_and_route` | Crossing uses normal passability, diagonal rules, cost, movement points and visits. | G4, G5, G10 | partial: server and direction handling; helper owns its options | partial: RoE terrain cost, cobblestone movement across all eight edge/corner directions on both levels, selected mixed-road and road-to-terrain transitions, blocked steps and options lifetime | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md), [road directions and transitions](evidence/topology-road-directions-2026-11-14.md), [options lifetime](evidence/pathfinder-options-lifetime-2026-11-10.md); other road, object and terrain cases remain |
| `movement_and_route` | Route finding considers legal wrapped neighbors under normal knowledge rules. | G6 | partial: neighbor enumeration and conservative hidden-tile handling | partial: direct RoE paths and synthetic hidden terrain/coast comparisons | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md), [hidden pathfinding](evidence/topology-hidden-pathfinding-2026-11-10.md); other obstacles and knowledge rules unverified |
| `movement_and_route` | On-foot Flight and Water Walk can switch layers where both are available. | G6, G10 | partial: pathfinder AIR/WATER transitions and conditional landing validation | partial: pathfinding at all edges and corners; two executed seam transitions; TCP loopback replica planning and one executed switch | pending | [transient switches](evidence/topology-transient-switches-2026-11-10.md); [GUI crossings](evidence/topology-transient-gui-crossings-2026-11-14.md) exercise Water Walk and Flight in both directions across the seams and both pairs of corners; other effects, vehicles and GUI cases unverified |
| `movement_and_route` | Boat travel, embarking and disembarking follow normal rules at seams. | G8 | partial: wrapped pathfinder and server movement; server rejects coast-cutting and sailing disembarkation onto visitable cells on toroidal maps | partial: clear-shore embark and reverse disembark in all eight directions on both levels, sailing and guarded disembarkation across all edges and corners on both levels with forced victory and server-handled retreat, one blocked corner and rejected disembarkation onto a nonblocking event; GUI embark, sailing and disembark; open-water diagonal crossing of both seams in both directions on `961e235` | pending | [all embark directions](evidence/topology-embark-directions-2026-11-14.md), [guarded disembarkation](evidence/topology-guarded-disembark-2026-11-15.md), [boat record](evidence/topology-boat-movement-2026-11-09.md), [GUI boat fixture](evidence/topology-boat-gui-2026-11-10.md), [current launch and crossing](evidence/topology-preview-961e235-2026-11-14.md); other shore configurations and animation unverified |
| `authority` | Client route and server agree on legal wrapped steps. | G1–G4, G6, P3 | partial: pathfinder and server | partial: RoE direct steps, and one graphical loopback corner crossing with a packet received by the other client | pending | [demo record](evidence/topology-demo-integration-2026-11-09.md), [graphical loopback](evidence/topology-graphical-loopback-2026-11-14.md); other client routes and remote multiplayer unverified |
| `authority` | Blocked or illegal crossing is rejected without applying movement effects. | G5, G10, P3 | partial: terrain and occupancy checks, direction, movement capabilities and replicated server-confirmed source layers; older saves retain unknown-layer fallback | partial: rock blocks all eight outward directions on two levels in both pathfinder and server; forbidden exits on land, in boats and after airborne transit, local and TCP replica replanning and one rollback | pending | [blocked directions](evidence/topology-blocked-directions-2026-11-14.md), [server source layer](evidence/topology-server-movement-layer-2026-11-10.md), [replicated layer](evidence/topology-replicated-movement-layer-2026-11-10.md), [boat record](evidence/topology-boat-movement-2026-11-09.md); GUI, turn boundaries, legacy unknown layers and other rejection cases unverified |
| `authority` | A turn cannot end during AIR or WATER transit; a stranded hero remains recoverable. | G12 | partial: turn guard, last-points rejection, conditional known-landing check, timer-driven ordinary landing on a clear known path, in-place landing and known same-turn routes for `AVIATE` vehicles, and conditional return to recorded departure tiles with whole-group prevalidation | partial: TCP loopback Flight rejection, layer switch, directly driven timer landing, manual in-place landing, synthetic-stranding origin return and turn transfer on two TCP replicas; atomic multi-hero landing check; local AIR/WATER landing preservation, unknown terrain, already-stranded state, combined abilities and timer fallback; synthetic `AVIATE` vehicle settlement and one- and two-step landing | pending | [transient turn](evidence/topology-transient-turn-2026-11-10.md), [known landing](evidence/topology-known-landing-2026-11-10.md), [timer landing](evidence/topology-timer-landing-2026-11-10.md), [in-place landing](evidence/topology-in-place-landing-2026-11-10.md), [aviation settlement](evidence/topology-aviate-in-place-2026-11-14.md), [aviation routes](evidence/topology-aviate-multistep-2026-11-14.md), [timer after real movement](evidence/topology-timer-live-order-2026-11-14.md), [stranding reproduction](evidence/topology-stranded-fog-reproduction-2026-11-14.md), [conditional origin return](evidence/topology-transient-origin-return-2026-11-14.md), [group recovery](evidence/topology-multiple-transient-return-2026-11-14.md); general recovery, wall-clock scheduling, remote-network play and graphical play unverified |
| `transient_route_safety` | Flight and Water Walk preserve safe landing while respecting explored terrain and legitimate routes. | G12 | partial: server requires a known landing after a transit step when one was known before; shared check filters requested client paths and cursor nodes for the current uninterrupted prefix | partial: actual spells stop before single-island stranding; returned path to a visible shore executes near fog; hidden-terrain and guard pairs; single-island Water Walk and seam Flight parity; synthetic unknown frontier on two TCP replicas | pending | [unknown-frontier guard](evidence/topology-unknown-frontier-guard-2026-11-14.md), [in-place landing search](evidence/topology-flight-in-place-search-2026-11-14.md), [client/server route parity](evidence/topology-transient-route-parity-gap-2026-11-14.md), [GUI crossings](evidence/topology-transient-gui-crossings-2026-11-14.md); raw graph consumers, future turns, interactive effects, additional GUI unsafe-route scenarios and general recovery remain; GUI Water Walk and Flight each declined one insufficient-points route and completed its positive control |
| `transient_emergency_relocation` | Recover an on-foot stranded hero to suitable known land without exposing fog or triggering visits. | G12 | partial: same-level known-land fallback with origin preference and group prevalidation | partial: synthetic old-version memory and disk archives, hidden and occupied land, multiple heroes, 252×252 fixture and TCP loopback replicas; direct timer updates select alternative land after origin loss and retry after simulated visibility change | pending | [emergency known land](evidence/topology-emergency-known-land-2026-11-14.md), [timer fallback](evidence/topology-timer-emergency-fallback-2026-11-14.md), [hidden event gap](evidence/topology-hidden-event-recovery-gap-2026-11-14.md); no safe known destination, vehicles, GUI and other dynamic states remain |
| `vision_and_guards` | Sight, fog and adjacency guards work across edges and corners without leaking hidden information. | G7, G11 | partial: range, unrestricted object visibility, Visions bonus, known-guard callbacks and route planning, Dimension Door preview and AI continuation, Scuttle Boat target preview, and layer-specific transit | partial: server corner sight and serialized move on two replicas with team/level isolation, object visibility, hidden range, synthetic hidden/visible guard callbacks, routes and cursor, hidden spell targets on bounded and toroidal maps, guard positions, removal and object relocation on server and serialized replicas, hero movement/removal with a synthetic directional mask, ground/air transit, guarded-water rejection and battle starts on entry to a guarded cell across all edges and corners on both levels | pending | [guarded crossings](evidence/topology-guarded-crossings-2026-11-15.md), [team and level isolation](evidence/topology-level-isolation-2026-11-09.md), [unrestricted object visibility](evidence/topology-unrestricted-object-visibility-2026-11-10.md), [hidden guards](evidence/topology-hidden-pathfinding-2026-11-10.md), [unique guard list](evidence/topology-unique-guards-2026-11-14.md), [spell preview review](evidence/topology-spell-fog-review-2026-11-14.md), [hidden boat target](evidence/topology-hidden-scuttle-target-2026-11-10.md), [hidden Dimension Door terrain](evidence/topology-hidden-dimension-door-2026-11-10.md), [guard battle and removal](evidence/topology-guard-battle-2026-11-09.md), [moved-object guard cache](evidence/guard-cache-moved-objects-2026-11-15.md), [hero guard cache](evidence/hero-guard-cache-2026-11-15.md), [Visions range](evidence/topology-visions-spell-2026-11-09.md); other unchecked queries, spell validation, real clients, confidentiality and rendered fog unverified |
| `vision_and_guards` | Object interaction across seams follows normal rules. | G10; additional object cases to specify | partial: server movement, visit logic and JSON approach masks | partial: gold resource visits including selective allowed and forbidden approaches in all eight directions on both levels, direct monster attacks with forced attacker victory and opposing-hero attacks with forced victory for each side across all edges and corners on both levels, neutral unguarded mine capture and guarded mine dialog, refusal, battle and capture across an edge; one played-out graphical hero battle initiated by AI across a seam | pending | [selective approaches](evidence/object-template-directions-2026-11-15.md), [resource directions and movement costs](evidence/topology-resource-directions-2026-11-14.md), [monster and hero attack directions](evidence/topology-monster-attack-directions-2026-11-15.md), [guarded mine visit](evidence/topology-guarded-mine-2026-11-14.md), [AI battle](evidence/topology-nullkiller-battle-2026-11-15.md); broader object audit and graphical battle cases unverified |
| `vision_and_guards` | Grail placement and digging remain valid at seams. | Grail-specific case to specify | partial: wrapped placement radius, hole lookup and server digging | partial: placement at one edge, wrapped hole exclusion, corner crossing and successful digging with in-memory state restoration | pending | [Grail record](evidence/topology-grail-placement-2026-11-09.md), [hole lookup review](evidence/topology-hole-index-review-2026-11-14.md); puzzle rendering, GUI, disk save and other levels unverified |
| `vision_and_guards` | Nearest-town spell effects use toroidal distance. | spell case to specify | partial: shared nearest-town helper | partial: unskilled Town Portal and direct Reinforcements effect, bounded and wrapped, including one cross-level Town Portal | pending | [nearest-town record](evidence/topology-nearest-town-2026-11-09.md); Reinforcements full cast, transfers and GUI casting unverified |
| `vision_and_guards` | Ranged adventure spell targeting uses wrapped displacement. | spell cases to specify | partial: common ranged check and Nullkiller2 Dimension Door candidate enumeration | partial: range check, one AI landing plan, server Dimension Door casts across all edges and corners on both levels and Scuttle Boat across an edge | pending | [ranged spell record](evidence/topology-ranged-spells-2026-11-09.md); GUI casts and other spell-specific cases unverified |
| `vision_and_guards` | Summon Boat selects and places a boat using topology-aware distance and adjacent water. | spell cases to specify | partial: nearest boat and boat-generator placement | partial: server-side casts including one cross-level selection, a wrapped-distance tie and last-used priority, plus one shipyard build on synthetic maps | pending | [Summon Boat record](evidence/topology-summon-boat-2026-11-09.md); GUI, other level arrangements and multiplayer unverified |

## Presentation and interaction

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `rendering` | Continuous terrain, roads, rivers, objects, overlays and fog at every edge and corner. | V1 | partial: viewport, renderer and overlay coordinate mapping; static seam-spanning footprints fitting one map period can be saved | partial: geometry, overlay coordinate and object-mask checks, player edge observations, dirt-road seams, two mixed-road transitions and a corner turn in static views, sampled river/terrain seams, one wrapped town sprite, a converted demo map, and wrapped-monster battle position matching | pending | [viewport geometry](evidence/topology-viewport-2026-11-09.md); [overlay copies](evidence/topology-overlay-copies-2026-11-10.md); [visual seam fixture](evidence/topology-visual-seams-2026-11-10.md); [mixed roads](evidence/mixed-road-turns-2026-11-15.md); [wrapped town](evidence/topology-wrapped-footprint-vmap-2026-11-10.md); [monster battle position](evidence/topology-wrapped-monster-battle-2026-11-10.md); other overlay types, seam art and continuous frames need testing |
| `rendering` | View spanning multiple map periods renders correct copies without erroneous duplicates. | V4 | partial: repeated tile and overlay mapping | partial: zoomed-out world view shows matching hero, town and resource icons on repeated copies; overlay coordinate and object-mask tests | pending | [wide zoom record](evidence/topology-gui-wide-zoom-2026-11-10.md), [overlay copies and before/after GUI](evidence/topology-overlay-copies-2026-11-10.md); other terrain, overlays, zoom levels and animation unverified |
| `viewport_input` | Independent camera scrolling crosses seams and remains responsive at low speeds. | V1; manual-scrolling case to specify | partial: wrapped camera and fractional keyboard/border scroll accumulation | partial: keyboard endpoints across both axes at normal and reduced speeds, accumulation tests, and user-recorded camera movement across both axes | pending | [manual scrolling record](evidence/topology-adapted-demo-2026-11-10.md), [user recording](evidence/topology-user-screen-recording-2026-11-10.md); border input, frame pacing and other surfaces unverified |
| `viewport_input` | Hover and clicks on visible copies address the same canonical location. | V2, V4 | partial: coordinate mapping | partial: one mouse-selected destination traverses both seams; corresponding horizontal copies activate the same route; hover on vertical and diagonal city copies shows the same status | pending | [mouse route](evidence/topology-gui-mouse-route-2026-11-09.md), [wide zoom](evidence/topology-gui-wide-zoom-2026-11-10.md), [repeated city hover](evidence/topology-repeated-city-hover-2026-11-14.md); other copies and object types unverified |
| `viewport_input` | Destination selection remains correct across both seams and at wide zoom. | V2, V4; destination-specific case to specify | partial: coordinate mapping | partial: one mouse-selected route across both seams and one repeated-copy selection | pending | [mouse route](evidence/topology-gui-mouse-route-2026-11-09.md), [wide zoom](evidence/topology-gui-wide-zoom-2026-11-10.md); other destinations and scales unverified |
| `viewport_input` | Directional hero shortcuts select the adjacent tile across a seam. | directional-input case to specify | partial: wrapped destination before path selection | partial: human crossed both axes and one corner in both directions, including a later run on build `4255569` | pending | [directional input record](evidence/topology-directional-input-2026-11-09.md); [horizontal session](evidence/topology-gui-directional-2026-11-09.md); [corner session](evidence/topology-gui-corner-2026-11-09.md); [current build](evidence/topology-adapted-demo-2026-11-10.md); remaining corners unverified |
| `movement_presentation` | Hero and boat cross as a short continuous step with correct direction and path indicator. | V3; boat-animation case to specify | partial: camera, sprite offsets and periodic image fragments | partial: game-state direction, sampled geometry and software compositing; slow hero/camera intermediate frames at an edge and corner; normal-speed hero video samples in four cardinal directions and one corner, plus sampled boat video in four directions; all recorded frames of one cropped corner interval inspected on an older build | pending | [sprite geometry](evidence/topology-viewport-2026-11-09.md); [slow GUI samples](evidence/topology-adapted-demo-2026-11-10.md); [hero seam video](evidence/topology-hero-seam-video-2026-11-10.md), [boat GUI](evidence/topology-boat-gui-2026-11-10.md), [current corner samples](evidence/topology-boat-corner-current-2026-11-10.md); 25 fps cropped boat samples cover direction changes during open-water corner crossings; full-viewport continuity, current-build frame sequences and other cases remain unverified |
| `minimap` | Visible area and map selection remain correct across one or both seams. | V5 | partial: split radar and shared logical raster, click and radar-boundary mapping | partial: interval arithmetic across 37,500 size pairs, radar intervals at four scales, SDL3 software raster and texture, selected Metal integer-upscaling checks and GUI camera selection at all four corners on 36×36 and 23×36 maps | pending | [pixel alignment and GUI](evidence/topology-minimap-pixel-alignment-2026-11-14.md), [radar alignment](evidence/topology-minimap-radar-alignment-2026-11-15.md), [rectangular map GUI](evidence/topology-rectangular-vmap-2026-11-15.md), [player GUI record](evidence/topology-gui-player-2026-11-09.md); fractional-scale radar rendering and GUI selection, SDL2 and other GPU backends unverified |
| `minimap` | Visible hero icons crossing an edge appear on the opposite side, including corners. | icon seam case to specify | partial: periodic icon copies with existing visibility filtering | partial: axis coverage and SDL3 software-canvas pixels at four corners and an interior position; player-specific hero list checks hidden, revealed and re-hidden enemies at every corner on both levels, including server fog packets to two TCP replicas at one underground corner; current GUI starts and crosses a corner but its full-window screenshot is insufficient to resolve icon fragments | pending | [hero icon and visibility evidence](evidence/minimap-wrapped-hero-icons-2026-11-15.md), [TCP fog packets](evidence/topology-tcp-movement-2026-11-10.md), [current preview](evidence/topology-preview-d61b3bd-2026-11-15.md); actual GUI icon fragments over fog, movement-driven visibility and other backends unverified |
| `minimap` | Object ownership updates use the correct cells across seams. | object-update case to specify | partial: owner-change refresh and editor minimap use wrapped footprint cells | partial: synthetic corner footprint mapping | pending | [object tile record](evidence/topology-object-tile-queries-2026-11-10.md); GUI update unverified |

Full-map fog range filtering and server reveal/hide are also checked on
bounded and toroidal maps with two levels and two teams. See the
[full-map fog record](evidence/full-map-fog-range-2026-11-14.md) for the
scope and the remaining multiplayer and rendering limitations.

Two graphical clients on one Mac also joined a loopback lobby, started the
demo map, executed a corner crossing, transferred the turn, moved the second
hero, reloaded the autosave in a new lobby and crossed the corner back. The
[graphical loopback record](evidence/topology-graphical-loopback-2026-11-14.md)
documents the limited observations; remote play and full state agreement remain
unverified by this run.

Nullkiller2's planned moves and resource visits across all seams and corners
on both levels also reached the real server request handler in shared-state
test fixtures; this does not yet establish a full AI turn or a graphical
multiplayer session. See the
[AI route record](evidence/topology-ai-direct-route-2026-11-09.md).

The same server fixture now also exercises Nullkiller2 plans using active
Water Walk and Flight bonuses across all seam directions and both levels,
ending on land. The [transient movement record](evidence/nullkiller-transient-seam-2026-11-15.md)
also checks planned Flight and Water Walk casts, accepted and rejected server
acknowledgements, and landings across all seam directions on both levels;
an autonomous planning turn collected visible gold over the west seam using
each spell on both levels. A subsequent turn test starts with limited sight,
checks movement-driven fog reveal and AI notification on both levels, and
confirms the other level remains hidden. Other autonomous routes remain unverified.
After six test-driven server steps across a seam, the same record checks an
unsafe seventh step is rejected and the AI gateway returns to the island for
both abilities and levels. Its rejection of a water destination is not by
itself a test of route-safety filtering.

Nullkiller's danger-map path searches now use the AI player's fog for
neighbors and known guards even when simulating an enemy hero. Synthetic
threat checks cover all edges and corners on both levels with a bounded
control and deliberately different team visibility; see the
[threat perspective record](evidence/nullkiller-threat-observer-2026-11-15.md).
In a synthetic corridor, a visible enemy also prompts an escape plan, and an
AI planning turn reaches that plan's destination through an in-process
server across all edges and corners on both levels. See the
[escape record](evidence/nullkiller-escape-seams-2026-11-15.md). Live enemy
discovery, graphical defence and other defensive responses remain unverified.
Known teleport channel endpoints in enemy threat planning now also use the
AI's perspective, with bounded and toroidal danger-map checks; see the
[teleport observer record](evidence/nullkiller-teleport-observer-2026-11-15.md).

For one Dimension Door landing across a seam, a hidden guard and no guard
produce matching first-plan cost, danger and loss; revealing an overwhelming
guard prevents the plan. The separate hidden destination is not planned in
either case. A synthetic visible armed object previously exposed a difference
between hidden-guard and no-guard plans; toroidal landing planning now uses
the player's visible guards. The [guard and fog record](evidence/nullkiller-dimension-door-guard-fog-2026-11-15.md)
describes both comparisons and their limits.

The later [preview run](evidence/topology-preview-eeb3540-2026-11-15.md)
completed two Nullkiller2 turns with crossings on both axes, then reloaded an
autosave and crossed a corner. The compared fog and state were limited to the
red player's recorded viewport and fields. The separate
[clean demo fixtures](evidence/demo-test-mod-clean-2026-11-15.md) exclude
the test-only entities found in the earlier prototype; one was started and
played through a corner crossing.

The newer [preview at `155b38b`](evidence/topology-preview-155b38b-2026-11-15.md)
also loaded the clean demo and crossed a corner after collecting gold. Its
direct launch is verified in the test environment; ordinary Launch Services
startup and human visual acceptance remain open.

A newer [local preview at `fd38877`](evidence/topology-preview-fd38877-2026-11-15.md)
loads the clean map when invoked directly. A helper prepares bundle-local
library paths, but Launch Services still waited while opening the existing
save directory, so ordinary Finder launch remains unverified.

After the user granted TB5 access, Launch Services opened the later
[`ab35306` local preview](evidence/topology-preview-ab35306-2026-11-15.md)
to its main menu. This was a separate startup check, not a Finder double-click,
gameplay in that run, or verification of later rebuilt bundles.

The [preview at `f0f7f4a`](evidence/topology-preview-f0f7f4a-2026-11-15.md)
subsequently started via Launch Services, loaded the versioned Crossroads
map and crossed its northwest corner. One static view showed both roads at
the corner junction; live AI spell planning and visual smoothness remain
unverified in this run.

A generated legacy H3M defaults to bounded mode and rejects direct crossings
in all eight directions on both levels; the same steps succeed after an
explicit in-memory toroidal opt-in. See the
[bounded seam regression](evidence/bounded-seam-regression-2026-11-14.md).

The signed graphical preview at `06ea341` ran two Nullkiller players on the
clean toroidal demo through red's day-nine victory. Server movement packets
show selected crossings on both axes. A separate headless run on the same
sparse fixture reached day 224 without finishing and was stopped. See the
[AI game record](evidence/topology-ai-game-06ea341-2026-11-15.md); these
runs do not prove general AI play or remote multiplayer.

A [seeded comparison](evidence/nullkiller-seeded-ai-2026-11-15.md) reproduced
repeated no-task turns and town-defender reservation with seed 4 in both a
toroidal fixture and its bounded control. The runs were stopped before a
winner, so this is neither proof of permanent deadlock nor proof of identical
causes. One other seed eventually won after multiple no-task turns.

After framed-packet support, two current test listeners also negotiated an
unframed lobby with the original external server, selected a bounded map and
exchanged chat. The [legacy-server record](evidence/current-client-legacy-server-2026-11-15.md)
does not establish a graphical or remote session on the current build.

Repeated client transit-route checks reuse validated prefixes within one path
graph and reuse a search workspace within each route check. A known-landing
search can stop after settling a valid landing; recovery searches needing an
actual route still calculate the full graph. The
[timing record](evidence/transient-landing-performance-2026-11-14.md) measures
one synthetic case; first-route and server costs remain unbounded by that
result.

## Persistence, compatibility and supported profiles

| Requirement ID | Trackable obligation | Planned scenarios | Implementation | Verification | Acceptance | Target evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `persistence_and_network` | Save/reload preserves topology and authoritative state. | P1 | partial: versioned header, replicated movement-layer and server serialization, experimental save marker | partial: server save/load, airborne layer round trip, prior-server-state migration, one synthetic origin recovery from an on-disk save, clipped occupancy round trip, boat identity and corner sailing, prior GUI save/reload, two-team two-level fog with replica movement after disk reload, TCP lobby reload with two test listeners and a subsequent underground corner crossing, and graphical loopback autosave reload including the framed protocol | pending | [replicated layer](evidence/topology-replicated-movement-layer-2026-11-10.md), [server save and fog replicas](evidence/topology-server-save-fog-replicas-2026-11-14.md), [TCP save reload](evidence/topology-tcp-movement-2026-11-10.md), [origin recovery disk save](evidence/topology-return-origin-disk-save-2026-11-14.md), [clipped occupancy save](evidence/topology-clipped-save-2026-11-10.md), [boat save](evidence/topology-boat-movement-2026-11-09.md), [save marker](evidence/topology-save-marker-2026-11-10.md), [demo GUI](evidence/topology-adapted-demo-2026-11-10.md), [GUI save after Flight](evidence/topology-transient-gui-crossings-2026-11-14.md), [graphical loopback reload](evidence/topology-graphical-loopback-2026-11-14.md), [framed graphical reload](evidence/network-framed-packets-2026-11-15.md); full state, in-transit GUI saves and remote graphical reload remain unverified |
| `persistence_and_network` | Supported multiplayer peers agree on movement and visibility. | P3 | partial: capability negotiation and server broadcast | partial: two TCP listeners negotiate and start from a VMAP, observe corner resource visit, turn change, legal seam moves and a rejected seam move, with position, movement points and both levels of fog agreement; collected gold and removal of its object agree across the server and both replicas before and after save/reload; a new TCP lobby reloads the server save and two newly connected listeners agree on surface and underground crossings; two graphical clients played and reloaded on loopback with the framed protocol | pending | [TCP movement, lobby and save reload](evidence/topology-tcp-movement-2026-11-10.md), [graphical loopback](evidence/topology-graphical-loopback-2026-11-14.md), [framed graphical loopback](evidence/network-framed-packets-2026-11-15.md); graphical observations do not compare complete replicated state or fog, and remote hosts, broader movement, full state and confidentiality remain unverified |
| `persistence_and_network` | Unsupported peers or readers cannot silently interpret topology incorrectly. | P4 | partial: tagged toroidal VMAPs and saves, tagged lobby capability and versioned packet framing | partial: VMAP round trips, save, packet, game preparation and TCP loopback tests; original engine rejects a tagged map header and save; mismatched packet layouts and invalid frames are rejected; one original graphical client joins a new host but cannot start its toroidal map | pending | [old network peer](evidence/topology-old-network-peer-2026-11-15.md), [old-reader check](evidence/topology-old-reader-2026-11-14.md), [framed protocol](evidence/network-framed-packets-2026-11-15.md), [capability design](network-capability-design.md); untagged prototypes, old servers, other versions and remote multiplayer remain unverified |
| `network_layout_validation` | Negotiated packet layouts are checked before payload decoding. | P4, two-peer framed TCP | partial: per-connection framed mode after acknowledgement and disconnect on decode failure | partial: matching and mismatched codecs, damaged headers, pre-ack ordering, two-peer TCP and bad-peer isolation; two graphical clients start, exchange movement and reload on loopback | pending | [framed protocol](evidence/network-framed-packets-2026-11-15.md); remote and malformed-server cases remain unverified |
| `bounded_compatibility` | Existing maps and saves remain bounded; toroidal behavior is opt-in. | P2 | partial: versioned default | partial: legacy header, bounded save, H3M-to-VMAP conversion, bounded pathfinder and server rejection of direct crossings at all edges and corners on both levels, and one graphical bounded game with an original host and experimental guest | pending | [legacy editor record](evidence/topology-legacy-editor-2026-11-10.md), [mixed-version bounded game](evidence/topology-old-network-peer-2026-11-15.md), `HeroMovementTest.LegacyBoundedMapsRejectDirectSeamStepsOnBothLevels`; full legacy gameplay regression unverified |
| `legacy_footprint_wrapping` | Converted legacy object cells wrap across edges and corners without being discarded on empty terrain or obstacle overlap. | E4–E6 | partial: wrapped occupancy, selected interactions, authoring rebuild and VMAP serialization; oversized footprints rejected | partial: synthetic and two real-map two-level round trips, loaded resource interaction, one real town sprite observed, town appearance lookup, generated heroes at wrapped towns and converted demo started; default rebuild retains explicitly shown fully clipped occupancy during topology changes and reindex; new tagged saves preserve it, while older saves cannot reconstruct missing state | pending | [clipped occupancy rebuild](evidence/topology-clipped-rebuild-2026-11-10.md), [clipped occupancy save](evidence/topology-clipped-save-2026-11-10.md), [wrapped VMAP and visual check](evidence/topology-wrapped-footprint-vmap-2026-11-10.md), [town anchor lookup](evidence/topology-town-anchor-terrain-2026-11-10.md), [main town startup](evidence/topology-main-town-start-2026-11-10.md), [real legacy round trips and launch](evidence/topology-legacy-editor-2026-11-10.md); interactive-overlap policy and general compatibility unverified |
| `legacy_footprint_wrapping` | Position-targeted scenario objectives resolve objects moved across seams. | objective cases to specify | partial: wrapped objective lookup and editor move/undo tracking | partial: five town-targeted condition types, selected hero targets, VMAP startup and one control evaluation; invalid-level and bounded checks | pending | [objective resolution and editor moves](evidence/topology-wrapped-objectives-2026-11-10.md); other win/loss execution and overlapping targets unverified |
| `authoring` | A supported authored map can have coherent passable seams. | E1 | partial: topology setting, wrapped brushes and flood selection, terrain redraw, object selection/preview geometry and main-town/objective reference updates, adapted demos, seam-interaction warning and unstable-visit-order save guard | partial: terrain corner and line turns, flood selection geometry and horizontal GUI terrain/road save, road-source and redraw geometry, topology switch, editor save/reopen, town and objective move/undo with game startup, horizontal seam town selection and movement in the editor, demo GUI traversal and writer guard; a populated two-player scenario round-trips and starts with one AI turn; new diagnostics and continuous drag preview unverified | pending | [Crossroads scenario](evidence/toroid-crossroads-2026-11-15.md), [flood selection](evidence/editor-fill-seams-2026-11-15.md), [adapted demo](evidence/topology-adapted-demo-2026-11-10.md), [main town startup](evidence/topology-main-town-start-2026-11-10.md), [objective moves](evidence/topology-wrapped-objectives-2026-11-10.md), [editor road redraw](evidence/topology-editor-road-redraw-2026-11-10.md), [object selection and GUI](evidence/topology-editor-object-selection-2026-11-10.md), [visit-order guard](evidence/topology-wrapped-visit-order-2026-11-10.md); scenario balance, other scenario references and existing-map seam repair unverified |
| `authoring` | Any supported random generator respects topology and template connections. | E2, E3 | not started | not run | pending | [generator design candidate](template-generator-design.md); generator and individual template support are conditional |
| `ai` | Each advertised supported AI can plan and act using legal wrapped routes. | A1 | partial: Nullkiller2 neighborhoods, exploration heuristics, portal planning, confirmed initial layer, destination-object precheck, accessible cluster targets and optional object-graph actors | partial: synthetic visibility changes when selecting and enumerating cluster targets, blocker lookup and retained goal description after removal, object-graph links across all edges and corners on both levels with bounded controls; scripted routes and interaction precheck, nearest-town planning including an occupied wrapped-nearest town checked against a server cast, server-backed Town Portal selection and rejected cast, bounded and wrapped patrol paths, AI actor layer selection, and real Nullkiller2 actions across both seams including a played-out hero battle; later preview completed an AI turn across the north-south seam | pending | [AI route record](evidence/topology-ai-direct-route-2026-11-09.md); [hidden cluster targets](evidence/nullkiller-hidden-cluster-2026-11-15.md); [cluster blocker lifetime](evidence/nullkiller-cluster-blocker-2026-11-15.md); [object-graph actors](evidence/nullkiller-object-graph-2026-11-15.md); [replicated layer](evidence/topology-replicated-movement-layer-2026-11-10.md); [patrol radius](evidence/topology-ai-patrol-2026-11-10.md); [Town Portal planning](evidence/topology-ai-town-portal-2026-11-09.md); [Town Portal selection](evidence/nullkiller-town-portal-selection-2026-11-15.md); [game session](evidence/topology-gui-directional-2026-11-09.md); [AI battle](evidence/topology-nullkiller-battle-2026-11-15.md); [latest preview](evidence/topology-preview-d6d9135-2026-11-15.md); optional object-graph mode is disabled in the shipped difficulty settings; full AI audit and profile support undecided |
| `ai` | Unsupported AI profiles are identified as unsupported. | A2 | partial: editor and scenario information warn that AI support is unverified | partial: translation files checked and scenario-list warning observed in current GUI | pending | [player status record](evidence/topology-ai-status-player-2026-11-10.md), [editor status](evidence/topology-editor-status-2026-11-09.md), [demo GUI](evidence/topology-adapted-demo-2026-11-10.md); profile-specific support and other displays unverified |

River redraw also preserves unrelated terrain flags and replaces the previous
river orientation on bounded and toroidal maps; see the
[regression evidence](evidence/river-rotation-flags-2026-11-15.md). The
editor's dirt-road and water-river brushes select equivalent sprite groups
and reflections for all 256 neighboring-cell combinations at each seam and
corner on both levels; see the
[neighborhood comparison](evidence/line-neighborhoods-2026-11-15.md).
Nullkiller skips an empty blocked-object cluster rather than dereferencing a
missing center; the isolated test covers bounded and toroidal maps, while the
complete object-removal sequence remains unverified. See the
[empty-cluster evidence](evidence/nullkiller-empty-cluster-2026-11-15.md).
The later [current preview](evidence/topology-preview-ef53b71-2026-11-15.md)
also exercised a player corner crossing, a battle initiated by Nullkiller, an
AI corner crossing and the return to the player on a clean demo map.
[Guarded-shore tests](evidence/topology-guarded-disembark-2026-11-15.md)
exercise disembarkation across an edge and a corner on both levels, including
the server battle and forced-victory return with the boat left at sea.
[Guarded-mine tests](evidence/guarded-mine-directions-2026-11-15.md)
exercise legal seam approaches, refusal, forced victory and retreat on both
levels, with bounded controls; some starting positions overlap the mine and
are synthetic. The uncovered bounded-server destination-direction discrepancy
was fixed and covered by the [server check](evidence/bounded-approach-direction-2026-11-15.md).
[Prisoner release](evidence/prison-release-guard-cache-2026-11-15.md)
refreshes the guard cache for the old and new hero footprints; synthetic
bounded and toroidal coverage checks the server and two serialized replicas.

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

The separate [`961e235` preview bundle](evidence/topology-preview-961e235-2026-11-14.md)
was built and signed, but its client has not yet reached an observable game
window; it is not current-build gameplay evidence.

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

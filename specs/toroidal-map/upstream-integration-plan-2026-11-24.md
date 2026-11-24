# Upstream integration: investigation and execution plan

Status: **execution started on user instruction**. A checked planning item below
records an observation, not a passed test of the eventual integrated build.
See the [initial compatibility inventory](evidence/upstream-compatibility-inventory-2026-11-24.md).

## Snapshot and scope

- Feature branch HEAD at inspection: `ced7f796e35ea400ab907d2e589960b5e53ef525`;
  this is a source revision, not a claim that this exact revision was released.
- The fork PR's base `fork/develop`: `141d0036974c8b6b4dfdb9faca2d56d691465f06`.
- Original VCMI `origin/develop`: `580fe835778bd5a542d869045fe1f06aad555ed7`.
- Fork base is an ancestor of that upstream revision; there are 140 commits
  reachable upstream after the fork base, including merges (31 first-parent
  commits). The checkout is shallow, so avoid unrelated all-history counts.
- The existing [fork PR #1](https://github.com/SoundBlaster/vcmi/pull/1) compares
  the feature with **the fork's** `develop`, not current upstream. GitHub
  reports it as conflicting.
- The local read-only `git merge-tree --write-tree --name-only <base> <feature>`
  trial found 3 conflicted files against the fork base and 12 against the
  recorded original upstream. These are file counts, not a measure of semantic
  changes, build success or test coverage. Automatically merged files also
  require focused review. No conflicts have been resolved.
- This integration planning branch starts at the released feature head and
  contains planning documentation only. Re-fetch and re-evaluate exact refs
  when execution is authorized; upstream may have moved.

## Conflict inventory

| File | Observed collision | Priority / next investigation |
| --- | --- | --- |
| `lib/serializer/ESerializationVersion.h` | Both branches allocated the same ordinals for different formats and capabilities. | **Critical:** design save, lobby and packet compatibility before editing the enum. |
| `lib/mapping/CMap.cpp` | Upstream checks visitability from top; the fork uses a wrapped displacement and directional masks. | **Critical:** retain correct seam direction with the new object-type rules; compare bounded behavior. |
| `lib/mapObjects/ObjectTemplate.cpp` | Upstream removes template-level visit directions; fork reads and tests those directions. | **Critical:** trace JSON/H3M objects, type handlers, scripts, RMG and existing fork-specific behavior; decide supported semantics from evidence. |
| `client/adventureMap/CMinimap.cpp` | Upstream replaces hero-only minimap control with icons; fork maintains scaled toroidal copies. | High: preserve icon behavior, fog and repeated updates in both map modes. |
| `config/schemas/settings.json` | Upstream requires `minimapShowIcons`; fork requires `minimapShowHeroes` and `globeCurvature`. | High: trace defaults, migrations and consumers before choosing settings compatibility. |
| `mapeditor/mainwindow.cpp` | Upstream factors editor startup for headless generation; fork customizes startup/logging and editor behavior. | High: preserve interactive and headless startup, plus toroidal authoring. |
| `lib/mapping/MapFormatH3M.cpp` | Upstream changes handler return type; fork checks unsupported predefined heroes. | Medium: reconcile API and preserve deliberate errors or an equivalent safe behavior. |
| `lib/json/JsonBonus.cpp` | Upstream and fork independently implement `AT_LEAST` / `AT_MOST`. | Medium: compare parsing, serialization and tests; avoid retaining duplicates without verification. |
| `lib/texts/CGeneralTextHandler.cpp` | Upstream assertion intersects the fork's language-detection changes. | Medium: preserve language additions and verify array assumptions. |
| `server/TurnTimerHandler.cpp` | Trial reports a whitespace-only conflict at the inspected hunk. | Low: preserve behavior; check surrounding upstream timer fix. |
| `test/CMakeLists.txt` | Different test source files were added at the same location. | Low syntactically: retain both relevant suites and verify discovery. |
| `Mods/vcmi/Content/config/translations/russian.json` | Upstream message edits overlap toroidal and compatibility messages. | Low syntactically: retain supported keys and valid JSON/localization behavior. |

The first three fork-base conflicts are minimap, settings and serialization.
The original-upstream trial additionally includes the other nine files above.
This list is tied to the exact snapshot, not a prediction for a future HEAD.

## Compatibility facts requiring a decision before implementation

**User decision, 2026-11-24:** The fork is a beta without active users; old
experimental save and network version numbers need not remain compatible when
integrating upstream. Allocate new toroidal versions after upstream's current
versions. Incompatible old data and peers must be rejected safely rather than
silently decoded under the wrong layout. This decision does not waive normal
bounded-map compatibility or authorize rewriting existing user files.

In the inspected enum, fork versions `TOROIDAL_MAP` through
`TOROIDAL_TELEPORT_RESOLUTION` use ordinals 940–946. Upstream uses 940 for
`MAP_ALLOWED_DIFFICULTIES`, 941 for `LOBBY_CAMPAIGN_ON_SERVER`, 942 for
`CAMPAIGN_STATIC_DATA`, and 943 for `VISITABLE_FROM_TOP_IN_OBJECT_TYPE`.
The published fork release `da6c93a` contains the experimental sequence.
Appending both enum lists does not by itself prove compatibility: version
checks also select concrete save and wire layouts.

The fork already writes a high-word `0x5452` marker for **new toroidal saves**,
tags its lobby greeting, has framed toroidal packets, and uses a separate
tagged VMAP version. These are branch-local discriminators, not upstream
allocations, and must be interpreted together with the old low-word layout.
Older untagged prototype saves/VMAPs can be ambiguous. Tagged saves still need
an explicitly verified old-to-new layout policy after version renumbering;
the existence of a marker is not proof that a new reader supports them.
New bounded saves follow a separate legacy-version rule. See
[save format](save-format-design.md) and [network capability](network-capability-design.md).

Preserve originals of real saves and map files. Investigate discriminators,
old readers and writers, version thresholds, and network negotiation; record
which combinations can be supported and which must fail explicitly. Do not
silently reinterpret ambiguous data, rewrite saves, or claim migration is
possible until demonstrated. If preserving a released format cannot be done
safely, present the concrete trade-off to the user before making that choice.

## SpecPM and documentation impact

The current package (`specpm.yaml`, `toroidal-map.spec.yaml`) validates at the
feature snapshot. Existing requirements describe behavior, not fixed ordinal
allocations. Do **not** rewrite a requirement merely because a merge fails.
Decide whether a normative clarification is needed after the compatibility
investigation; keep proposals and confirmed behavior distinct.

| Contract or design | Review or planned update after investigation |
| --- | --- |
| `specs/toroidal-map.spec.yaml`: `persistence_and_network`, `network_layout_validation`, `bounded_compatibility` | Specify supported/unsupported save and peer boundaries if existing statements are insufficient; add evidence only after checks pass. |
| `save-format-design.md` | Record tagged released saves vs untagged prototypes, old and new version mappings, migration/rejection rules, and bounded-save behavior. |
| `network-capability-design.md` | Document old/new tagged peers, untagged upstream peers, framing and lobby version negotiation; separate actual compatibility from intended behavior. |
| `toroidal-map.spec.yaml`: `movement_and_route`, `vision_and_guards`, `legacy_footprint_wrapping` | Review object-type visitability, approach directions and equivalent bounded/seam behavior; avoid assuming old template masks are preserved by upstream. |
| `topology-design.md` | Record any supported interaction or format changes, boundaries and legacy-map consequences. |
| `toroidal-map.spec.yaml`: `minimap`, `viewport_input`, `rendering`, `editor_toroidal_navigation`, `authoring` | Check minimap settings/icons, wrapping, globe input, editor startup and headless generation. Add requirements only if an actual agreed contract gap exists. |
| `template-generator-design.md` | Assess upstream RMG and editor CLI changes against the experimental generator; do not mark template integration complete from a successful merge. |
| `implementation-matrix.md` and `review.md` | Reassess affected verification on the merged revision; mark old-revision evidence stale **for the integration build**, not retroactively invalid for its original revision. Map focused scenarios and remaining limits. |
| `TODO.md` and new evidence records | Track each completed stage, exact commits, commands, platforms, results, failures and remaining risks. |

## Planned tasks — all pending execution

- [x] **PLAN-01 — Baseline.** Capture feature/fork/upstream revisions, run
  merge-tree analysis, inventory conflicts and map the current SpecPM package.
  `specpm validate . --json` passed with zero errors/warnings on the feature
  snapshot; it is not an integrated-build result.
- [ ] **INT-01 — Refresh and isolate.** Re-fetch without recursively changing
  submodule worktrees; record new exact heads, worktree state and release
  artifacts. Repeat the two merge-tree comparisons and update the inventory.
  Use this dedicated integration branch; preserve the released feature branch
  and do not force-push or merge upstream into it yet.
- [ ] **INT-02 — Compatibility design first.** Trace version-dependent reads,
  writes, tags and packet negotiation; construct a version table using original
  bounded saves, released tagged toroidal saves and VMAPs, older untagged
  prototypes, current upstream and the intended new build. Include old/new
  client/server directions, lobby greetings and framed packets. Reproduce with
  actual artifacts when available; do not fabricate evidence or overwrite them.
  Write the proposed mapping or safe rejection policy in the relevant design
  and SpecPM documents before changing serialization code.
- [ ] **INT-03 — Merge in isolation.** Merge the selected upstream commit into
  the integration branch without rewriting either history. Resolve files by
  semantic groups with focused commits where Git permits; a merge conflict
  resolution is not complete until the merge commit is created. Record the
  exact merge commit, unresolved cases and any upstream changes superseding
  fork fixes. Do not modify the upstream repository or open an upstream PR.
- [ ] **INT-04 — Objects and movement.** Reconcile visitability, object-type
  handlers and wrapped displacement, including bounded-map controls and real
  supported H3M/VMAP data. Recheck server and pathfinder agreement, guards,
  diagonal approaches, all seams/corners and both levels. Add focused tests
  for demonstrated failures and preserve or explicitly revise fork behavior.
- [ ] **INT-05 — UI, editor and RMG.** Integrate minimap icons and settings,
  globe projection and input, translations, interactive editor startup and
  upstream headless map-generation CLI. Check toroidal and bounded controls,
  settings upgrade, map open/save and representative seam editing; assess
  generated maps and template connectivity separately from the core merge.
- [ ] **INT-06 — Other conflicts and auto-merges.** Reconcile H3M hero error
  handling, bonus aliases, language detection, turn timer and test registration.
  Review automatically merged network packs, game state, AI pathfinding,
  SDL3 rendering, CMap/VMAP formats, RMG and editor code for API and semantic
  changes; Git's clean merge is not evidence of behavioral compatibility.
- [ ] **INT-07 — Focused verification.** Build the selected integration commit
  using the repository's actual build instructions and TB5 preflight. Run
  relevant existing save (`SaveFileTest`), connection
  (`GameConnectionTopologyTest`), topology (`MapTopology`), minimap/view
  (`MapViewModelTopologyTest`), object/guard and editor tests; add meaningful
  regressions for failures and run upstream visitability and bonus tests.
  Check the compatibility matrix from INT-02, including explicit rejection and
  bounded-map controls, rather than just a round trip with the new build.
- [ ] **INT-08 — Runtime and handoff.** Check representative client/server
  movement and saves, seam/corner interactions, editor editing, graphical
  startup and bounded-map controls. Separate local automated, graphical,
  multi-client and remote multiplayer evidence; do not claim a full regression
  or visual continuity from a still image. Update SpecPM, matrix and evidence
  to reflect only results obtained. Push the integration branch to the user's
  fork and report outstanding risk before any release or branch promotion.

## Execution dependencies

`INT-01 → INT-02 → INT-03 → INT-04/05/06 → INT-07 → INT-08`. The order keeps
the compatibility decision ahead of conflict resolution; UI and other semantic
reviews may be investigated in parallel after that decision. Any automatic
merge result, old test result, build artifact or existing released save is
evidence only for its own revision and environment until rechecked.

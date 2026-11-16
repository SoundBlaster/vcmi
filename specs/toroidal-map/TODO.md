# Toroidal map: remaining work

This is the execution checklist for the experimental toroidal map goal, as
agreed on 2026-11-15. The [implementation matrix](implementation-matrix.md)
tracks requirement coverage, verification and acceptance separately; update
it and link evidence as work progresses. An unchecked item may have partial
coverage already and does not imply that all of its cases are unimplemented.

## Verification scope

Prioritize the common topology mechanisms and representative seam scenarios:
movement and obstacles, interactions, boats and transient movement, fog,
authority and persistence, AI, and bounded-map compatibility. Test both
axes and corners where the geometry itself differs; add focused regression
tests for actual failures. This is not an exhaustive regression of every
game object, map, spell, combination or AI profile. Record unsupported or
untested cases without presenting representative checks as comprehensive.

Earth-specific investigations should resolve defects that affect the demo
or reveal a general topology problem. Exhaustive portal-route permutations
are not an acceptance gate. Keep core topology, independent legacy fixes and
demo preparation reviewable as separate changes where practical.

## 1. Earth seam playthroughs

- [x] Check human-controlled north/south crossings in both directions in the
  graphical Earth game, including continuation after save and reload:
  [staged Earth fixture](evidence/earth-gui-vertical-save-2026-11-15.md).
- [x] Check all four corner crossings and their reverse directions in the
  graphical Earth game: [staged land corners](evidence/earth-gui-corners-2026-11-15.md).
- [x] Record map, build, commands, observed hero state and remaining gaps.
  A staged horizontal land crossing, save and reload are already documented
  in [the seam record](evidence/earth-gui-seam-save-2026-11-15.md).

## 2. Movement and interactions

- [x] Review existing evidence for representative seam scenarios involving
  impassable terrain, roads, guards, interactive objects, boats, embarkation
  and disembarkation: [blocked terrain](evidence/topology-blocked-directions-2026-11-14.md),
  [road costs](evidence/topology-road-directions-2026-11-14.md),
  [resource visits](evidence/topology-minimum-size-movement-2026-11-15.md),
  [embarking](evidence/topology-embark-directions-2026-11-14.md) and
  [guarded disembarking](evidence/topology-guarded-disembark-2026-11-15.md).
  A [focused repeat](evidence/representative-seam-review-2026-11-15.md)
  passed 54 tests. These are representative synthetic scenarios, not all
  objects and rules.
- [x] Review representative Flight and Water Walk seam routes and relevant
  movement effects; reproduce observed failures with focused regression tests:
  [interactive crossings](evidence/topology-transient-gui-crossings-2026-11-14.md),
  [layer switches](evidence/topology-transient-switches-2026-11-10.md) and
  [route agreement regression](evidence/topology-transient-route-parity-gap-2026-11-14.md).
  Recovery without a known landing and hidden-event recovery remain excluded.
- [x] Record the user's decision to leave the rare case with no known suitable
  safe landing unresolved under current rules. This does not resolve the case;
  general recovery and the hidden-event case remain outside the agreed scope.

## 3. Persistence, server and multiplayer

- [x] Verify authoritative position, movement and fog after representative
  seam crossings and save/reload with client/server agreement:
  [two-level TCP replica checks](evidence/topology-tcp-movement-2026-11-10.md#two-levels-after-tcp-save-and-reload).
  These use loopback and do not compare every game-state field.
- [x] Play and reload a network game with two graphical clients:
  [framed graphical loopback](evidence/network-framed-packets-2026-11-15.md#two-graphical-clients-and-reload-with-framing).
  Both clients were on the same Mac; remote hosts and full replica-state
  comparison remain unverified.
- [x] Check representative incompatible peers and save formats are rejected
  explicitly and a mixed-version bounded game still starts and moves:
  [original network peer](evidence/topology-old-network-peer-2026-11-15.md)
  and [original format reader](evidence/topology-old-reader-2026-11-14.md).
  This does not cover every version, remote host or mixed-version save reload.

## 4. Earth demo and authoring

- [ ] Compare generated template-zone connections with actual toroidal hero
  routes and guards. The [initial Jebus Cross probe](evidence/jebus-cross-seam-probe-2026-11-16.md)
  identifies static candidate seam links. A [two-route experiment](evidence/two-route-template-2026-11-16.md)
  captures generation-time zone IDs and finds unguarded seam transitions.
  Test-only guarded seam placement, obstacle closure, save/reload and server
  approaches have been exercised on one map. The existing inland passages
  remain and trial placement is unreliable. Preserve the original internal
  connections and add explicitly declared seam connections as distinct edges,
  including when they connect the same zone pair. Prevent other inter-zone
  exits without disconnecting zones; same-zone seam continuity needs no guard
  just because it crosses the seam. Validate routes for both players and
  measure fairness over multiple maps; establish reproducible inputs.
  An opt-in placement heuristic now prefers the declared geometric seam
  contacts and fails if it cannot find them. It does not reserve or construct
  passages; independent post-processing failed for three tested seeds. One
  sample preserved server routes from both starts to all four traced original
  guards. An experimental mode now reserves seam approaches before object
  placement and biases nearby placement away from them. Complete the
  template-to-guard integration, graph validation and failure handling.
- [ ] Generate terrain features continuously across the toroidal seams and
  corners. Consider seam-first placement and optional symmetry; verify terrain
  transitions, roads, rivers, objects and movement without requiring mirrored
  tiles. Review the rendered result in addition to checking map data.
- [ ] Implement continuous toroidal map-editor navigation, including terrain,
  object layers, selection and brush input across both axes and corners.
  The minimap must show the coordinate-reference rectangle and all wrapped
  viewport portions. Verify real editor interactions and retain bounded-map
  behavior. Repeated viewport sections and canonical input are implemented;
  A north-west camera crossing, one resource move through a corner, one town
  move through the western seam, one terrain undo, and both opposite corners
  of a bounded control were checked. Verify other corner directions, object
  placement and additional multi-tile moves, redo,
  broader bounded behavior and smoothness before closing this task.
  [Clipboard placement](evidence/editor-clipboard-placement-2026-11-16.md)
  now canonicalizes toroidal destinations and checks hero placement against
  the actual target; real copy/paste and drag gestures still need checking.
  [Initial minimap geometry](evidence/editor-minimap-reference-2026-11-16.md)
  and its reference outline are implemented and observed in a corner view.
  [Repeated viewport section geometry](evidence/editor-repeated-sections-2026-11-16.md)
  is connected to rendering and editor input. See the
  [initial interactive check](evidence/editor-toroidal-scroll-2026-11-16.md).
- [ ] Review Earth draft object footprints and collisions for issues affecting
  actual play, preserving the source and a reproducible local test copy.
  A [mask audit](evidence/earth-object-overlap-audit-2026-11-15.md) identifies
  new interactive overlap candidates; a staged engine check reaches each of
  one pair of creature banks and opens an activity. A current-draft scan finds
  no shared visitable cells between non-hero, non-boat objects; other footprint
  collisions remain unverified.
- [x] Check representative land, water and region connectivity and the effects of replacing
  the original underground gates. The [current pair and approach check](evidence/earth-current-gates-2026-11-16.md)
  records their intentional behavior change; wider regional connectivity and
  normal routes to each gate remain unverified. A [road-to-gates pathfinder
  inspection](evidence/earth-start-connectivity-2026-11-15.md#gate-paths-after-reaching-the-road)
  found no direct route in one modified game state. A [server route through one
  gate](evidence/earth-start-connectivity-2026-11-15.md#executed-route-from-the-road-through-one-gate)
  then succeeded after forced victories and other test-only changes.
- [x] Investigate the [water-monolith mismatch](evidence/earth-portal-road-2026-11-15.md):
  restore the misplaced entrance to water in the demo preparation recipe.
- [ ] Address remaining placement and collision problems if they affect demo
  play. A [blocked seam monolith approach was opened](evidence/earth-object-overlap-audit-2026-11-15.md#seam-monolith-approach-repaired)
  by removing a decoration; a server route from the original hero reached it
  with forced wins and other test-only changes.
  A [mine approach was opened](evidence/earth-object-overlap-audit-2026-11-15.md#mine-approach-repaired)
  by removing a reef; a staged server visit captured it after a forced battle.
  An [autonomous AI run exposed a misplaced whirlpool](evidence/earth-autonomous-whirlpool-2026-11-16.md):
  the recipe now puts all six exits on water and a staged server check reaches
  each by boat. A complete post-fix AI game remains unverified.
  [All three water-monolith exits and one graphical route](evidence/earth-portal-road-2026-11-15.md#graphical-water-portal-and-other-exits)
  are checked from the relocated entrance; exhaustive reverse-route checks
  are deferred unless a defect or a demo requirement warrants them.
  A [seeded AI run and reproduction](evidence/earth-ai-seed42-movement-rejection-2026-11-16.md)
  exposed a rejected movement-layer request and a related occupied-boat
  pathfinder disagreement; the latter is fixed and checked, although the
  original run did not log the requested layer.
- [x] Prepare a road that crosses a seam and verify its logical and static
  visual continuity: [Earth road extension](evidence/earth-road-seam-2026-11-15.md#static-view-and-local-demo-integration).
  Its far end is not connected to another existing road network. A
  [diagnostic route from the start](evidence/earth-start-connectivity-2026-11-15.md#prepared-access-to-the-seam-road)
  reaches and crosses it after two decoration removals and forced battles;
  continuous animation and an unassisted playthrough remain unverified.
- [x] Make the demo convenient to launch and play with an isolated profile;
  keep third-party map and game assets outside the repository unless their
  distribution rights have been established. The [current draft starts
  through the human scenario list and accepts a first move](evidence/earth-human-start-2026-11-15.md);
  a [direct original-start launcher](evidence/earth-human-start-2026-11-15.md#direct-launch-from-the-original-start)
  also starts the map for a human in an isolated test profile;
  the [seam preview](evidence/earth-seam-preview-2026-11-16.md#direct-human-preview-launch)
  also starts directly for a human. A [Finder double-click](evidence/earth-human-start-2026-11-15.md#finder-launch-of-the-repaired-map)
  of a launcher copy started the repaired map; an unassisted route to the
  seam remains unverified.

## 5. AI and bounded compatibility

- [x] Review Nullkiller2 playthroughs and targeted planning, interaction and
  movement checks across edges and corners. The [short graphical match](evidence/topology-crossroads-ai-multiday-2026-11-15.md)
  and [seeded comparisons](evidence/nullkiller-seeded-ai-2026-11-15.md)
  establish representative behavior, not general AI support or an AI audit.
  Seed 4's long-running scenario is unresolved without a demonstrated
  topology-specific cause. Continue treating AI support as experimental.
- [x] Verify representative ordinary-map and older-save behavior for the
  profiles under test: [mixed-version bounded game and old save](evidence/topology-old-network-peer-2026-11-15.md),
  plus the [current suite and save repeat](evidence/full-suite-40bffe9-2026-11-16.md).
  This does not establish compatibility with every map, mod or save.

## 6. Verification and acceptance

- [ ] Run appropriate automated and graphical checks for each change, update
  the SpecPM evidence and implementation matrix, and record revisions and
  environment. Keep known failures and unverified scenarios visible.
- [x] Review representative interface checks: [mouse route across both seams](evidence/topology-gui-mouse-route-2026-11-09.md),
  [minimap selection](evidence/topology-minimap-pixel-alignment-2026-11-14.md)
  and [static Earth road](evidence/earth-road-seam-2026-11-15.md#static-view-and-local-demo-integration).
  These do not prove every input, rendering backend or continuous animation.
- [ ] Have the user assess scrolling and movement animation continuity when
  convenient; an [isolated seam preview](evidence/earth-seam-preview-2026-11-16.md)
  starts the hero beside the road crossing. This does not block the remaining
  engineering work but is needed for visual acceptance.
- [ ] Commit each major code, specification or documentation step and push
  the feature branch to the user's fork. Declare the feature complete only
  after the agreed functional and visual acceptance has been established.

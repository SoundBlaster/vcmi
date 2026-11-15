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

- [ ] Review Earth draft object footprints and collisions for issues affecting
  actual play, preserving the source and a reproducible local test copy.
- [ ] Check land, water and region connectivity and the effects of replacing
  the original underground gates. Document intentional gameplay changes.
- [x] Investigate the [water-monolith mismatch](evidence/earth-portal-road-2026-11-15.md):
  restore the misplaced entrance to water in the demo preparation recipe.
- [ ] Address remaining placement and collision problems if they affect demo
  play. [All three water-monolith exits and one graphical route](evidence/earth-portal-road-2026-11-15.md#graphical-water-portal-and-other-exits)
  are checked from the relocated entrance; exhaustive reverse-route checks
  are deferred unless a defect or a demo requirement warrants them.
- [x] Prepare a road that crosses a seam and verify its logical and static
  visual continuity: [Earth road extension](evidence/earth-road-seam-2026-11-15.md#static-view-and-local-demo-integration).
  Its far end is not connected to another existing road network; continuous
  animation and normal access from the starting position remain unverified.
- [ ] Make the demo convenient to launch and play with an isolated profile;
  keep third-party map and game assets outside the repository unless their
  distribution rights have been established. The [current draft starts
  through the human scenario list and accepts a first move](evidence/earth-human-start-2026-11-15.md);
  Finder launcher execution and a normal route to the seam remain unverified.

## 5. AI and bounded compatibility

- [x] Review Nullkiller2 playthroughs and targeted planning, interaction and
  movement checks across edges and corners. The [short graphical match](evidence/topology-crossroads-ai-multiday-2026-11-15.md)
  and [seeded comparisons](evidence/nullkiller-seeded-ai-2026-11-15.md)
  establish representative behavior, not general AI support or an AI audit.
  Seed 4's long-running scenario is unresolved without a demonstrated
  topology-specific cause. Continue treating AI support as experimental.
- [ ] Verify that ordinary maps and older saves remain bounded and that
  existing gameplay is preserved for the profiles under test.

## 6. Verification and acceptance

- [ ] Run appropriate automated and graphical checks for each change, update
  the SpecPM evidence and implementation matrix, and record revisions and
  environment. Keep known failures and unverified scenarios visible.
- [ ] Check available interface behavior, including tile and route selection,
  minimap and static seam rendering. Do not treat one screenshot or game-state
  observations as proof of continuous animation.
- [ ] Have the user assess scrolling and movement animation continuity when
  convenient; this does not block the remaining engineering work but is needed
  for visual acceptance.
- [ ] Commit each major code, specification or documentation step and push
  the feature branch to the user's fork. Declare the feature complete only
  after the agreed functional and visual acceptance has been established.

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

- [ ] Review existing evidence for representative seam scenarios involving
  impassable terrain, roads, guards, interactive objects, boats, embarkation
  and disembarkation; fill consequential gaps on either level as needed.
- [ ] Review representative Flight and Water Walk seam routes and relevant
  movement effects; reproduce observed failures with focused regression tests.
- [ ] Keep the documented rare case where no known safe landing exists under
  the current game rules. General recovery and the hidden-event case are not
  prerequisites for the next work unless the agreed scope changes.

## 3. Persistence, server and multiplayer

- [ ] Verify authoritative position, movement and fog after seam crossings
  and save/reload, including client/server agreement in relevant scenarios.
- [ ] Extend multiplayer checks beyond the existing TCP replica and graphical
  loopback coverage to a real game with graphical clients; record precisely
  which hosts, scenarios and replicated state were checked.
- [ ] Check that incompatible peers and save formats are rejected explicitly
  where required, without breaking supported bounded games.

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
- [ ] Prepare a road that crosses a seam and verify its logical and static
  visual continuity. The current Earth draft's roads do not reach its edges.
- [ ] Make the demo convenient to launch and play with an isolated profile;
  keep third-party map and game assets outside the repository unless their
  distribution rights have been established.

## 5. AI and bounded compatibility

- [ ] Review Nullkiller2 playthroughs and targeted planning, interaction and
  movement checks across edges and corners; resolve consequential gaps and
  document supported scope without claiming a complete AI audit.
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

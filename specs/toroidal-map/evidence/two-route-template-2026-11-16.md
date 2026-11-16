# Two-route guarded template experiment

The test-only `vcmi-test:Two Route Experiment` template proposes the symmetric
graph `1—3—2—4—1`: zones 1 and 2 are starts, and 3 and 4 are treasure
zones. All four connections request guard value 12,000. Starting-zone mine and
treasure settings are copied from zone 1 to zone 2; zone 4 copies zone 3's
mine and treasure settings. Fly, Dimension Door, Water Walk, Angel Wings and
Boots of Levitation are banned for the experiment. This is a design candidate,
not a claim that these rules are balanced or that all bypasses are prohibited.
The test fixture is not shipped as a playable template.

An opt-in generator test on macOS arm64 generated a 108×108 one-level map,
two players, no water and requested seed 12345. It reloaded the VMAP and
checked that the five specified bans survived serialization. The map generated
with ordinary bounded placement; a diagnostic copy in memory was switched to
toroidal and its occupancy rebuilt. The saved local map remains bounded and
is not a playable toroidal result. Generated content uses the test fixture
registry, so this VMAP must not be treated as a standalone release map.

The diagnostic observer captures zone IDs at generation time, before the
generator discards its temporary zone state. On this sample, the engine found
20 distinct adjacent seam cell pairs between different zones for which both
cells are empty land, both are unguarded according to wrapped monster lookup,
and `CMap::canMoveBetween` accepts each direction. Seventeen pairs join zones
2 and 4; three join zones 1 and 3. These are *template connections that were
specified as guarded*, but these alternative seam contacts have no monster
guard according to this static check. One cardinal example is `(0, 4)` in
zone 2 to `(107, 4)` in zone 4. This checks the immediate transitions; it
does not establish an end-to-end hero route from either starting town or
prove the absence of every dynamic interaction.

The corresponding [walk grid](data/two-route-sample.walk),
[zone grid](data/two-route-sample.zones) and
[bidirectionally checked transitions](data/two-route-sample.seams) are
preserved. Each seam row contains `zoneA zoneB xA yA xB yB`. The saved map
SHA-256 is `95f9381bf4d5911414d4f897e80ee6dc2f9cdb807b0d317c5f26adef95c624f1`;
the zone-grid SHA-256 is
`cd65103bc1b05dcb03669f82f67c48fe89cd7ac03049b810a2dc9955ea2a555d`.
The seed does not guarantee an identical map with the local multithreaded
generator; the saved grids identify this particular sample.

`HeroMovementTest.MonsterOnSeamGuardsBothSidesAndDiagonalApproaches` passed
for all eight neighbors of one synthetic monster, with a separate existing
test confirming that a guarded crossing starts a battle. These results do not
prove that the random generator placed guards at all intended passages.
The selected generator/reload check, three monster-guard tests and six Python
comparison tests passed. The placement of routes east/west, path lengths,
actual guard armies and rewards, player fairness, complete bypass prevention
and real gameplay remain unverified.

The current generator does not place zones with toroidal geometry. A possible
next step is to use this zone/guard information to reject unintended seam
contacts, then generate explicit guarded crossings on both routes and verify
them with player pathfinding and server movement. Merely blocking every seam
would defeat the desired two-direction gameplay.

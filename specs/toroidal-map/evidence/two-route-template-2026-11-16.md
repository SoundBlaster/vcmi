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

## Guard and obstacle trial

The test-only trial now attempts to reserve two guarded crossings for one
starting zone, place actual blocking objects on remaining unguarded seam
contacts, and reload the resulting toroidal VMAP. A successful local sample
had two guards, four obstacle placements and no remaining contacts reported
by the static seam audit. Its VMAP SHA-256 is
`765467cdcdfdefc210165f5b7e7e20f47d48a092e4698808580090f2ff7b99d5`;
its guard-coordinate sidecar hash is
`a3828a0aa7858f154759454bfda29a4edcd10107e26d51d4f4c8b646079f1aa5`.
These local artifacts use test content and are not a release map.

After reload, server movement started the expected battle when approaching
each guard from either side. With fog revealed and movement artificially
increased, one starting hero followed server-executed paths of 42 and 28
nodes to the two guards. This proves those particular approaches work, not
that both players have equivalent routes or that all bypasses are closed.
The original inland connections remain, consistent with the later decision to
extend the original graph. The added guards are not yet tied to declared seam
edges, so the trial does not establish the intended extended graph. Obstacles are not yet proven to preserve all
interior routes. Guard selection in this trial is a fixed creature and an
approximate strength target, not the generator's normal balance algorithm.

Ten independent trial runs with the same requested seed produced one
successful placement; nine reported that no suitable seam crossing with a
clear approach was found. This sample is too small to estimate a general
success rate. The multithreaded generator did not yield the same layout on
each run, and the precise source of variation has not been isolated.
Diagnostic path and army measurements for candidate original guards vary;
the candidate-to-template-connection mapping and fairness are not verified.

The audit and obstacle planner are reusable code, but generation and guard
placement above are opt-in test instrumentation, not a supported generator
feature. A complete implementation must select intended seam connections
while constructing the zone graph, preserve intended inland connections and prevent unintended wrapped exits,
maintain zone and path invariants, and fail explicitly when it cannot satisfy
the contract. It must then validate both players' actual routes, armies and
rewards across multiple generated maps.

## Explicit additional seam edges

Following the map author's clarified model, the test-only two-guard trial now
specifies seam edges `2–3` and `2–4` before searching for positions. Both
pairs also have ordinary inland edges in the original fixture; the seam
edges are additional, not replacements. These declarations currently live
in the test code, not in the template schema or generator implementation.

On one local run the first generated map lacked a suitable `2–4` crossing,
and the second placed both requested guards and six obstacles. The static
seam audit found no remaining unguarded inter-zone contacts after save and
reload. The saved map's SHA-256 is
`9bd32b64764e3913114191de37b5264c40eaaddc36c37f318fc7b3cd4f410b4e`.
Server movement started battles from both sides of both new guards. With full
visibility and artificially increased movement, one starting hero followed
paths of 42 and 30 nodes to them. The other start did not have a path to
either new guard before battles. Four candidate original guards remained
reachable from their respective nearby starts in a diagnostic pathfinder
check, but their identity as the original template edges and preservation of
every original route were not established. No balance claim follows from
these measurements, and two attempts do not estimate the success rate.

## Experimental placement constraints and graph audit

`CZonePlacer` now has a diagnostic-only opt-in API for specifying pairs of
zones that should meet across a seam. Its capacity-balancing step prefers
assignments with at least three cardinal seam contacts for each requested
pair, then minimizes area error. It throws if the final zone assignment does
not satisfy that geometric condition. Three contacts need not be consecutive,
and this heuristic does not guarantee an open, guarded or usable passage.
The normal bounded generator does not set the constraint. There is no template
field or supported toroidal generator yet.

On this machine, for seed 2 the baseline placement had two cardinal contacts
between zones 3 and 4 and the constrained placement had ten; for seed 4 the
counts were one and eleven. Seed 14 had none and the constrained attempt
explicitly failed. These examples are not a statistical performance estimate.
In another sample of 20 distinct seeds, both requested pairs 2–3 and 2–4
already had at least three contacts even without the constraint. The four
zone areas in the first two constrained samples were within 84 tiles of one
another on a 108×108 map, but this does not establish general size balance.
An invalid requested zone also caused generation to fail.

An opt-in map with constrained 2–3 and 2–4 placement was subsequently
processed by the separate test-only guard and obstacle trial. On one saved
sample, the static audit found no unguarded ordinary land contacts between
distinct zones anywhere on the map, including the seams. Four server approach
tests and two server-executed routes from one player to the new guards passed.
The second player's access, preservation of every original graph edge,
interaction rules, guard removal, portals, spells and balance are not covered.
The static audit excludes guarded tiles and does not prove a complete hero
route graph. The trial still constructs guards and barriers after placement;
it is not a supported generator feature.

## Original connection trace and server routes

An opt-in generator trace records the template connection ID, endpoint zone
IDs and actual guard position when a direct guarded connection is placed.
It does not trace indirect connections, portals, unguarded passages or wide
connections. The trace is diagnostic and is not part of the map format.

For one saved test-content map, the trace identified all four original
template edges. After adding the two test-only seam guards and obstacles and
reloading the map, the server executed paths from each starting hero to the
two original guards associated with that player's zone. The paths required
33 and 36 steps for player 0 and 34 and 31 for player 1. Both added guards
were also reached by player 1 and attacked from both sides in separate server
tests. Fog was revealed and movement was artificially increased to 30,000;
the tests stop at the battle, so they do not prove traversal after victory.
This is one sample, not evidence of general balance or full graph preservation.

The saved sample VMAP SHA-256 is
`814dc2e487e019a996755e5c95d76f2c7ff1e2c9614fe9be066c715c2a0c635e`;
the trace sidecar SHA-256 is
`c9eec7493f191213907003e218c6e6b9caff76617a687c323ce14f5009129038`.
For seeds 1, 2 and 3, none of ten attempts per seed completed the separate
post-generation guard placement trial. A follow-up run found no suitable
candidate for 2–3 on seed 1; on seeds 2 and 3 it placed a 2–3 guard but found
no suitable candidate for 2–4. This small sample is not a general success
rate, but confirms that geometric seam contact alone is insufficient. Seam
passages and their approaches need to be reserved during generation, before
other objects consume or block the candidate area.

## Early seam approach experiment

An opt-in diagnostic now chooses cardinal seam contacts and connects their
approaches to each zone's free paths before zone-filling modifiers run. It
also adds the selected tiles to the object-distance field on both sides of
the rectangle. That distance field is a placement bias, not an enforceable
exclusion zone; the trial must still reject occupied or guarded approaches.
The ordinary generator does not enable this mode. Guard placement and closure
of other inter-zone contacts are still separate test-only post-processing.

For diagnostic seeds 1 through 10, one trial per seed placed the two selected
seam guards, closed all statically detected unguarded inter-zone land contacts,
and reloaded the toroidal map. Each sample then passed separate server checks:
both sides of each added guard start a battle, one starting hero can reach the
two added guards, and each starting hero can reach its two traced original
guards. Movement was artificially increased and fog revealed; the checks end
when battle starts, do not test after victory, and do not establish balance.
These ten outcomes are not a general success-rate claim. The trial uses test
content, a fixed guard type and heuristic obstacle placement, and must not be
treated as a supported playable generator.

The user's seam-first landscape sketch motivates selecting consistent terrain
features around the rectangle before filling interior areas. Opposite sides
are adjacent, so features can continue across the seam without being mirror
images; corners involve both axes. This experiment reserves movement paths
only. It does not yet generate matching terrain, roads or rivers across those
edges, nor has its visual result been reviewed.

The seed-3 sample was subsequently opened in an isolated real client and
played through an ore pickup and ore-pit capture in the starting zone; see
the [human play record](two-route-human-play-2026-11-16.md). Seam gameplay
and visual continuity were not reached in that run.

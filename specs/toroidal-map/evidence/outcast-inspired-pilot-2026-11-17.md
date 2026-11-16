# Outcast-inspired saved-map pilot

The local `Jebus Outcast.h3t` pack on the user's game disk has SHA-256
`d876825eff581265e60abd175aa65a4211b0cf970e55e77c69b91dfe8ac0598f`.
The pack labels version 3.01 and author MKC. It was inspected using the
separately obtained MIT-licensed `heroes3-template-util` at commit
`e0143ce06d5cb8d616d5facd4e05569437878a07`. The source file is not included
in this repository. A 2-player, single-level test template approximates a
subset of the pack's zones, treasures, mines, and connections. It is not
Jebus Outcast: full rules, objects, bans, underground, and balance have not
been converted or verified.

The pilot uses test-only seam reservations, guards and obstacle placement;
it is not a supported generator. The static contact audit must succeed before
the diagnostic map is saved. On one sample, 56 candidate unguarded contacts
were reduced to zero and two extra guarded crossings placed. Static checks do
not prove playability. In ten earlier generated samples, only one passed both
the start-to-seam-guard and eight original-guard route checks; another passed
only the seam checks. The seed is not a reproducibility guarantee because
parallel generation has previously produced different results for the same
seed. Reject failed maps and check the actual saved artifact.

For the successful 108x108 sample, server tests executed both starting heroes'
routes to their own seam guard and separately to all eight original guards.
Separate tests initiated battles against each seam guard from both adjacent
approaches. Tests reveal fog and increase movement points; they are not a full
playthrough, spell audit, fairness proof, or visual inspection. One failure
also reproduced on the bounded map before adding seam barriers, so its cause
cannot be attributed to the seam closure without further investigation.

The map was generated with the `vcmi-test` fixture mod. After filtering fixture
heroes, skills and spells, a later saved sample contained no `vcmi-test:`
identifiers in its JSON archive entries and passed both route checks and the
bidirectional guard-battle check again. This string check is not proof of
independence: a clean-profile game launch remains required. More work is
needed on reproducible artifact packaging, actual game
rules and magic, zone balance, graphical continuity, and full-match play.

The pilot also exposed loss of map-specific game settings during VMAP save.
The map format now persists overrides, with bounded and toroidal round-trip
tests. This permits the one-hero setting to survive a save; it does not by
itself validate every original template rule.

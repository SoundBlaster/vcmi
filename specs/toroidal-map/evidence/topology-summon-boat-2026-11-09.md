# Summon Boat across map seams

Summon Boat ranks available boats by Manhattan distance, ignoring level, after
giving priority to the caster's previously used boat. The distance now uses
the shortest wrapped displacement on a toroidal map; candidate filtering,
priority and tie behavior remain unchanged. The common boat generator also
canonicalizes an adjacent target before checking its terrain and occupancy.
Bounded maps retain their former behavior.

Two tests use a real game state and server-side spell cast on a synthetic
36×36 map. The first checks that bounded mode selects the interior boat,
whereas toroidal mode selects a boat across the seam. The second checks that
water across the seam is unavailable in bounded mode, then casts the spell
and verifies the boat appears on that canonical water tile in toroidal mode.
Both tests failed before their respective fixes. The final focused run passed
24 tests across Summon Boat, movement and Town Portal suites.

TB5 logs: `summon-boat-before-tests.log`, `summon-boat-final-build.log`,
`summon-boat-final-tests.log`. The common boat-generator change also affects
shipyards, but no shipyard interaction was exercised. Other levels, boat
priority and ties, GUI effects and multiplayer remain unverified here.

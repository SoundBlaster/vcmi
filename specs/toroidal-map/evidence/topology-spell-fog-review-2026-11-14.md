# Spell previews on bounded and toroidal maps

Review G1 identified that the player-side Dimension Door target check and
Scuttle Boat target check used fog-aware previews only for toroidal maps.
The same information boundary now applies on bounded maps as well. When
Dimension Door permits targeting through fog, a player-side preview of an
unknown destination does not distinguish its hidden terrain; the server's
unrestricted callback still validates the actual destination. When the
spell does not permit targeting through fog, the hidden destination is
rejected. Scuttle Boat rejects hidden targets when its effect does not ignore
fog; the unrestricted server callback retains the actual object check.

The Dimension Door guarded cursor also checks both the target and each guard
against the caster player's visibility for either map topology. The extra
guard check is intentional: on bounded maps `getGuardingCreatures` can still
return a hidden guard, and an unrestricted server callback can see hidden
guards on either topology. This review did not establish behavior for every
modded fog-ignoring object-removal spell or prove network confidentiality.

The focused tests cover visible and hidden guards, hidden land and water,
bounded and wrapped player/server callbacks, Scuttle Boat targets, and
Dimension Door casts across a seam and a corner. The Nullkiller2 exploration
fixture now checks that bounded planning respects its normal range while the
toroidal planner can select across the seam; a hidden rock/grass comparison
remains in the wrapped case. These are synthetic local tests, not graphical
or remote multiplayer observations.

On macOS arm64, the server and test targets built on TB5. The final full suite
passed 1,495 tests from 226 suites with four disabled and no skips. It included
the branch-era untagged save and VMAP and the Complete `Peacemaker.h3m`.
The log is `runtime/full-suite-spell-fog-review-final-2026-11-14.log` on TB5.
An earlier run failed because it selected a tagged save and retained two test
expectations for the old bounded-map preview; it did not establish a pass.
`specpm validate . --json` reported zero errors and warnings.

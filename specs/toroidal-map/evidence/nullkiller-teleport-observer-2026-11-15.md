# Teleport visibility in AI threat planning

`CPathfinderHelper` previously filtered teleport entrances, channel type and
exits using the owner of the hero whose path was being simulated. Nullkiller
also simulates enemy heroes while building its danger map, so those decisions
depended on the enemy's fog rather than the AI player's fog. A focused test
with different team visibility failed before the change; the result is
`runtime/teleport-observer-before-2026-11-15.xml` on TB5.

The helper now uses the configured knowledge observer for those visibility
queries, falling back to the hero owner if none is supplied. The actual hero
is still used to check whether a visible exit is passable and whether its
occupant is friendly. The change also affects bounded AI searches because
the same Nullkiller configuration is used there; ordinary searches that do
not specify an observer keep their previous behavior.

The focused checks cover:

- A two-way channel whose exit is visible to only one of the two teams,
  with the visibility assignment reversed. The default-owner helper and the
  explicit-observer helper see their own respective channels.
- A visible exit occupied first by an enemy and then by a friendly hero,
  judged relative to the moving hero's owner rather than the observer.
- An entrance with one or two known one-way exits, including the random-exit
  classification when the teams have different visibility.
- Nullkiller's actual danger-map computation on a water-separated synthetic
  map: a known enemy threatens a target through a visible two-way channel on
  both bounded and toroidal maps. Hiding the exit from the AI and showing it
  to the enemy removes that predicted threat.

The focused tests passed on macOS arm64, Release build based on `7f3256b`
with this change; `runtime/teleport-observer-final-2026-11-15.xml` is under
the checkout's TB5 build root. They do not establish complete AI fog
isolation, live portal discovery, graphical behavior, or remote multiplayer.
Nullkiller normally disables deterministic planning through one-way exits;
the one-way test checks the shared helper, not an autonomous AI turn.

The complete local test run passed 1,964 tests, skipped four optional
external map, save and legacy-server fixtures, and reported three disabled
tests. The TB5 log and XML for the final implementation are
`runtime/full-suite-teleport-observer-final-2026-11-15.log` and
`runtime/full-suite-teleport-observer-final-2026-11-15.xml`. A separate
focused recheck of 37 tests is in
`runtime/teleport-observer-recheck-2026-11-15.xml`. Client, server and
Nullkiller2 targets built; existing duplicate-library and missing Conan
search-path warnings remained. SpecPM validation reported no errors or
warnings. These builds are not a runtime graphical or multiplayer check.

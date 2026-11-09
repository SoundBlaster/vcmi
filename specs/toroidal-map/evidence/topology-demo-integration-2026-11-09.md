# Demo-based integration checks

On 2026-11-09 the Linux Heroes III demo was obtained from the archived URL
in `launcher/demo.cpp`, and its SHA-1 matched that code's expected
`74c28240794d0aa2fb52cadcd088ab6dd47478c1`. Only the files selected by that
launcher were extracted, without executing the installer, into the
checkout-specific TB5 build root. The game's bundled `roe-demo` mod was used.
The demo has limited content; this is not verification of Complete edition.

The full `vcmi-test` fixture includes extra entities requiring Complete data.
For this run only, the copied build-tree `vcmi-test/mod.json` was replaced by a
resource-only descriptor, while retaining the fixture's map files. The
original build-tree descriptor and test preset were backed up under
`runtime/demo-state`; the repository fixture was not changed. The demo files
were temporarily linked into the isolated TB5 VCMI user-data directory.

- Source baseline: `da734d1`, plus the source files identified below. macOS
  arm64, Apple clang 21.0.0, Release build. A full build of `vcmitest`,
  `vcmiclient`, `vcmiserver`, and `vcmieditor` exited 0; log
  `demo-final-build.log` in the TB5 build root. Existing linker warnings
  about duplicate libraries and absent Conan paths remain.
- Before the loader fix below, initialization with the demo crashed or
  failed before the test body. With the fix alone, five independent
  `vcmitest` processes each passed eight selected tests: their logs are
  `demo-index-only-process-1.log` through `demo-index-only-process-5.log`.
  They cover a 36×36 single-level RoE fixture: client-side pathfinding and
  server movement across each edge and each corner, movement cost compared
  with equivalent interior steps, rejection of one blocked step, a small
  wrapped sight range, the static-object restriction predicate, and JSON map
  format round-trip/version checks. They do not cover all terrain, vehicles,
  interactions, guards, levels or network conditions.
- `demo-header-compat-tests.log`: both the map JSON round trip and in-memory
  header serialization/default for the earlier serialization version passed.
- `demo-save-tests.log`: bounded and toroidal game state save/reload tests
  passed, including map topology and a hero's position. The saved hero did not
  cross a seam before this test; fog and multiplayer were not checked.
- With the loader index fix applied, the original bounded demo map ran
  headlessly with AI and exited 0 after reaching day 99
  (`demo-index-only-headless.log`). The log reports missing demo assets, content
  validation warnings, and AI task failures; it is not evidence that all
  demo features work, nor evidence of toroidal gameplay.
- An attempt to drive the editor UI did not reach an observable window via
  the available automation; no visual continuity or editor acceptance is
  claimed.

```text
abd8329206d495431625518b0480060d9572f0211e95b2acfb6dffe6872457c0  lib/IHandlerBase.h
ed88cfc69cbd78d06df681785d027b75c941374cca39e9e597a77e321eec013c  test/game/HeroMovementTest.cpp
f0d0fe5afce3ca16777e340b55666a9ce36e4be31fbf34357eedc06177c34140  test/game/SaveGameTest.cpp
1bdcacc330fbfd08c29b6ad87a8426c77ee9b6c506433c57725e15ac4c4e9cb6  temporary build-tree vcmi-test/mod.json
```

Original proprietary demo assets are not included in the repository.

## Complete data follow-up

The user's Windows Complete installation was copied to a separate TB5
runtime directory; the original was not modified. The test fixture descriptor
and test preset were restored before these runs. The source tree was based on
`da734d1` with the local changes recorded by the hashes above and the test
fixture and language-detection changes present in the working tree.
Those independent changes were subsequently recorded as `38ffb84` (legacy
index), `a8d5132` (language profiles), `d9738a9` (AI test locking), and
`1ee9663` (language-independent battle assertions).

- `complete-targeted-tests.log`: 12 tests passed, including movement on RoE
  and SoD fixtures, game-state save/reload, format checks, one portal AI case
  and the static-object restriction predicate.
- `complete-full-tests-language-detection.log`: running `./vcmitest` from
  the build's `cmake/bin` directory passed 1,321 tests on macOS arm64; three
  tests were disabled. Earlier complete-suite attempts exposed missing AI
  fixture read locks and English-only battle-log expectations; the test
  fixture changes address those issues. This suite is not visual acceptance.
- The copied demo map was converted into a synthetic toroidal `.vmap`, after
  relocating 98 static objects whose footprints violate the current pilot
  restriction. A headless AI run exceeded day 874 before it was stopped
  (`toroidal-demo-headless.log`). This smoke test does not establish visual
  continuity, correct AI play, or coverage of all seam interactions.
- The game-data language detector was extended to compare all eight existing
  profiles. With the copied Complete data the client log reports Russian,
  `CP1251`, and deviation `0.000388139`. The client interface language
  remained separately configured as English.

After `b5f5aad`, `HeroMovementTest.crossedSeamAndFogSurviveGameStateReload`
passed with Complete data (`save-seam-tests.log`). It performs a real server
step from (0,1) to (35,1), serializes and reloads the game state in memory,
and checks the destination, topology, one visible tile and one hidden tile.
This is not a full-file save after crossing or exhaustive fog verification.
The test source SHA-256 at this run was
`8108025b1e6abd2c7852d739c5b4c689f7ccf2506756d01ac79b315ec68b10ac`.

The real viewport, animated seam crossing, multiplayer and seam-spanning
static objects still require additional work or verification.

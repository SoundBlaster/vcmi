# Earth seam preview for visual review

`tools/stage_earth_seam_road.py` accepts the current source-checked Earth
demo in addition to its earlier fixture. It creates a separate map with red
hero `randomHero_7305` moved from anchor `(165,353,0)` to `(1,425,0)` and
changes the scenario name to `Earth Seam Road QA`. The initial preview's
SHA-256 is `057ab6f2363d1d86e06fb24faba924b8f56c91b0c19145a7bfd5565ec146df7f`;
its input is the demo SHA-256
`67640eefaa91e9efcf04315bef66197d834e662652bd25bd10f234391c43e5a8`.
Decoded object comparison found only the hero changed; the header and terrain
archive members are identical, and the localized scenario name differs.
The tool rejected a different input and refused to overwrite the preview.
The later [repaired preview](earth-object-overlap-audit-2026-11-15.md#seam-monolith-approach-repaired)
has a different SHA-256 and replaces it in the local launch profile; the
initial map remains available as a rollback copy.

The local profile is `runtime/earth-seam-preview-2026-11-16/` under the TB5
build root. It links the local game data and mods but has separate map,
configuration and saves. Initially `Start Seam Preview.command` launched the
local client for manual selection of `Earth Seam Road QA`; the later direct
launch is described below. The script's shell syntax was checked. Finder
double-click was not independently tested.
No source map or game assets are included in the repository.

On macOS arm64, client `1.8.0.7744dbd` started this profile through the normal
scenario list with the opt-in diagnostic channel. The map observation showed
252×576 and toroidal mode. The selected human hero's visitable position and
movement points were `(0,425,0)` / 1,700, then `(251,425,0)` / 1,625 after
west, then `(0,425,0)` / 1,550 after east. A static screenshot was observed;
the process was forcibly stopped afterwards. The observation and screenshot
are in the local profile. This prepares a convenient scene for the user's
visual review; it does not establish continuous animation, full-frame scroll
quality or balance of an ordinary game.

## Direct human preview launch

The client has an opt-in `--preview-map MAP` option that selects a local map
and starts it with a human player. It rejects an empty path and incompatible
AI-only, headless, spectator, automatic-turn, external-server, test-map and
test-save options before startup. The ordinary menu and existing AI-only
`--testmap` path are unchanged. `--testseed` remains restricted to `--testmap`.
The opt-in Python CLI test runs the built client against these invalid
combinations; its two test methods passed on macOS arm64.

On 2026-11-16 the client built from `8f1f4d3` plus this option started the
preview map directly in an isolated QA profile. The server and client logs
identified red as human; its observed hero crossed west from `(0,425,0)` to
`(251,425,0)`. The local preview profile's launcher was then updated to use
the new option and invoked from `/Users/egor`, a different working directory.
It started the same map directly with a human red player; the hero crossed
the seam west with movement points `1560 → 1485`. The observation is
`runtime/earth-seam-preview-2026-11-16/launcher-observe.jsonl` on TB5. Both
test clients were force-stopped. Neither launch checks Finder double-click,
clean shutdown, animation continuity or a complete game. The previous
menu-based launcher text is retained beside the local launcher for rollback.

The client was subsequently rebuilt from clean commit `7c42081`. The two
CLI test methods passed again, and a fresh isolated graphical preview started
red as human on the toroidal map. A west command moved the hero from
`(0,425,0)` to `(251,425,0)`, with movement points `1500 → 1425`.
Observation: `runtime/earth-quickstart-qa-2026-11-16/head-observe.jsonl` on
TB5. This client was force-stopped after the check.

An additional exploratory profile placed `headless`, `onlyai`, `spectate` and
`donotstartserver` under `session` in its ordinary settings file. A control
launch without preview did not load those inserted values: its logged
`onlyai` and `donotstartserver` values were false and a GUI was created.
Therefore the experiment does not demonstrate a persisted-session defect;
an attempted defensive change was removed before the clean build above.

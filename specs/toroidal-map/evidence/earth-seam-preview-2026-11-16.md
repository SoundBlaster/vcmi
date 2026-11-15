# Earth seam preview for visual review

`tools/stage_earth_seam_road.py` accepts the current source-checked Earth
demo in addition to its earlier fixture. It creates a separate map with red
hero `randomHero_7305` moved from anchor `(165,353,0)` to `(1,425,0)` and
changes the scenario name to `Earth Seam Road QA`. The current preview's
SHA-256 is `057ab6f2363d1d86e06fb24faba924b8f56c91b0c19145a7bfd5565ec146df7f`;
its input is the demo SHA-256
`67640eefaa91e9efcf04315bef66197d834e662652bd25bd10f234391c43e5a8`.
Decoded object comparison found only the hero changed; the header and terrain
archive members are identical, and the localized scenario name differs.
The tool rejected a different input and refused to overwrite the preview.

The local profile is `runtime/earth-seam-preview-2026-11-16/` under the TB5
build root. It links the local game data and mods but has separate map,
configuration and saves. `Start Seam Preview.command` launches the local
client; `README.txt` describes selecting `Earth Seam Road QA`. The script's
shell syntax was checked. Finder double-click was not independently tested.
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

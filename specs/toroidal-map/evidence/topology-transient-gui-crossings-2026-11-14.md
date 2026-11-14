# Interactive Flight and Water Walk crossings

The local macOS command-enabled client described in
[the command-channel record](diagnostic-command-channel-2026-11-14.md)
ran two disposable 36×36 toroidal maps using the ordinary adventure-map UI.
Its window title was `1.8.0.578e551`; the running package predates the final
command-channel input guard and close-on-exec hardening. The client route
filter and server known-landing checks were present. This is not an
interactive test of the exact final committed executable.

The map had Solmyr with 1,560 movement points and 100 mana, Water Walk and
Flight, and visible water separating a start tile from a land destination.
The spellbook and target tile were operated through the opt-in local command
channel. The UI showed route arrows and, after a second target click, the hero
on the destination. The observation trace recorded:

| Fixture | Spell | Visitable tile sequence | Mana | Movement |
| --- | --- | --- | --- | --- |
| Interior | Water Walk | `(15,16) → (15,17) → (15,18) → (15,19) → (15,20)` | 100 → 92 | 1,560 → 1,160 |
| Interior | Flight | `(15,16) → (15,17) → (15,18) → (15,19) → (15,20)` | 100 → 85 | 1,560 → 1,160 |
| East/west seam | Water Walk | `(34,16) → (35,16) → (0,16) → (1,16) → (2,16)` | 100 → 92 | 1,560 → 1,160 |
| East/west seam | Flight | `(34,16) → (35,16) → (0,16) → (1,16) → (2,16)` | 100 → 85 | 1,560 → 1,160 |

All coordinates are on level 0. Each spell was tested after starting or
restarting the scenario. The observation trace is
`runtime/observation-command-test.jsonl` under the TB5 VCMI build tree;
sequence numbers 7–12, 17–22, 26–34 and 37–44 respectively contain the four
runs. Screenshots were observed interactively; no continuous recording was
made, so this does not establish smooth animation. Clicking water without a
spell did not produce a route in the separate interior scenario; that does
not establish the result of a direct server request.

To regenerate the fixtures locally, use
`python3 specs/toroidal-map/tools/make_water_crossing_fixtures.py SOURCE OUTPUT_DIR`.
The source is the separately prepared `toroidal-route-safety-flight-waterwalk.vmap`
(SHA-256 `e33217ee91859cfcb95b1744e991ae2d1027c1bbe2da5a87acf8d751805c2199`),
currently in `runtime/complete-data/Maps` under the TB5 build tree. It is not
checked into this repository. The generator refuses to overwrite existing
maps and checks the expected terrain and hero. Its output uses plain grass
tiles without matching coast art and is diagnostic, not a polished map.

These are positive short crossings to already visible land with ample movement
points. They do not establish unsafe-route rejection in the GUI, crossings at
the other edges or corners, moving guards, changed fog, general stranded-hero
recovery, remote multiplayer, or animation smoothness.

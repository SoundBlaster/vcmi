# Interactive Flight and Water Walk crossings

The local macOS command-enabled client described in
[the command-channel record](diagnostic-command-channel-2026-11-14.md)
ran five disposable 36×36 toroidal maps using the ordinary adventure-map UI.
Its window title was `1.8.0.578e551`; the running package predates the final
command-channel input guard and close-on-exec hardening. The client route
filter and server known-landing checks were present. This is not an
interactive test of the exact final committed executable.

Each map had Solmyr with Water Walk and Flight, and visible water
separating a start tile from a land destination. Starting movement points
varied with the randomly chosen army, as recorded below.
The spellbook and target tile were operated through the opt-in local command
channel. The UI showed route arrows and, after a second target click, the hero
on the destination. The observation trace recorded:

| Fixture | Spell | Visitable tile sequence | Mana | Movement |
| --- | --- | --- | --- | --- |
| Interior | Water Walk | `(15,16) → (15,17) → (15,18) → (15,19) → (15,20)` | 100 → 92 | 1,560 → 1,160 |
| Interior | Flight | `(15,16) → (15,17) → (15,18) → (15,19) → (15,20)` | 100 → 85 | 1,560 → 1,160 |
| East/west seam | Water Walk | `(34,16) → (35,16) → (0,16) → (1,16) → (2,16)` | 100 → 92 | 1,560 → 1,160 |
| East/west seam | Flight | `(34,16) → (35,16) → (0,16) → (1,16) → (2,16)` | 100 → 85 | 1,560 → 1,160 |
| South/north seam | Water Walk | `(15,34) → (15,35) → (15,0) → (15,1) → (15,2)` | 100 → 92 | 1,500 → 1,100 |
| South/north seam | Flight | `(15,34) → (15,35) → (15,0) → (15,1) → (15,2)` | 100 → 85 | 1,560 → 1,160 |
| Southeast/northwest corner | Water Walk | `(34,34) → (35,35) → (0,0) → (1,1)` and back | 100 → 92 | 1,560 → 1,137 → 714 |
| Southeast/northwest corner | Flight | `(34,34) → (35,35) → (0,0) → (1,1)` and back | 100 → 85 | 1,560 → 1,137 → 714 |
| Southwest/northeast corner | Water Walk | `(1,34) → (0,35) → (35,0) → (34,1)` and back | 100 → 92 | 1,560 → 1,137 → 714 |
| Southwest/northeast corner | Flight | `(1,34) → (0,35) → (35,0) → (34,1)` and back | 101 → 86 | 1,500 → 1,077 → 654 |

All coordinates are on level 0. Each spell was tested after starting or
restarting the scenario. The observation trace is
`runtime/observation-command-test.jsonl` under the TB5 VCMI build tree;
sequence numbers 7–12, 17–22, 26–34, 37–44, 91–98, 102–110, 116–127,
130–141, 145–158 and 162–173 respectively contain the ten runs. Each corner trip was made in
both directions within the same turn. Screenshots were observed interactively; no continuous recording was
made, so this does not establish smooth animation. Clicking water without a
spell did not produce a route in the separate interior scenario; that does
not establish the result of a direct server request.

In subsequent restarts, each spell also made a round trip across each cardinal
seam within the same turn. Water Walk and Flight each followed
`(34,16) → (35,16) → (0,16) → (1,16) → (2,16)` and the reverse route, then
`(15,34) → (15,35) → (15,0) → (15,1) → (15,2)` and the reverse route on the
other fixture. Each run started with 1,560 movement points and ended with
760; Water Walk consumed 8 mana and Flight 15. Trace sequences 179–194,
197–210, 214–229 and 232–246 contain these four round trips, respectively.

To regenerate the fixtures locally, use
`python3 specs/toroidal-map/tools/make_water_crossing_fixtures.py SOURCE OUTPUT_DIR`.
The source is the separately prepared `toroidal-route-safety-flight-waterwalk.vmap`
(SHA-256 `e33217ee91859cfcb95b1744e991ae2d1027c1bbe2da5a87acf8d751805c2199`),
currently in `runtime/complete-data/Maps` under the TB5 build tree. It is not
checked into this repository. The generator refuses to overwrite existing
maps and checks the expected terrain and hero. Its output uses plain grass
tiles without matching coast art and is diagnostic, not a polished map.

In a separate restart of the seam scenario, the hero spent 1,300 points on
land, ending at `(34,15,0)` with 260 points, then cast Water Walk (100 → 92
mana). Clicking the visible opposite shore twice showed no route arrows and
did not move the hero; an east shortcut also left the hero in place. The
trace records the setup at sequences 48–90. The same four-step crossing cost
400 points in the positive runs above. This is a GUI observation: it does
not by itself identify the rejection mechanism or establish what a direct
server movement request would do. The east shortcut did not demonstrate a
safe one-step water transit; the ordinary GUI's automatic destination-layer
selection is a possible reason.

## Save and load after a Flight crossing

On the south/north fixture, the same client cast Flight and made the round
trip described above, then crossed southward once more. At `(15,2,0)`, with
360 movement points and 85 mana, the game reported that the new save was
successful. The hero then moved to `(16,2,0)` with 260 points. Loading that
save through the game interface restored `(15,2,0)`, 360 movement points,
85 mana, and `toroidal: true` in the observation trace (sequences 247–262).
The GUI showed the hero on the expected shore after loading.

In a separate restart, Flight reduced mana from 100 to 85, and the hero
crossed southward to `(15,2,0)` with 1,100 movement points. After saving and
loading that state in the GUI, the player selected the original shore without
recasting. The client displayed a route, and the hero followed
`(15,2) → (15,1) → (15,0) → (15,35) → (15,34)` with mana still at 85 and
700 movement points remaining. Trace sequences 264–279 cover this run.
The successful crossing demonstrates that the necessary Flight capability
was still effective in this particular loaded game. A save while actually
in transit and remote clients were not independently verified. The saves
were created in the local test profile and are not in the repository.

These cases do not establish moving guards, changed fog, general stranded-hero
recovery, remote multiplayer, or animation smoothness.

## Insufficient movement and a positive control

On the newer signed macOS client built from `cf4ca3b` with the diagnostic
cursor fix, the vertical crossing fixture started with the hero at
`(15,34,0)` and 1,560 movement points. After casting Water Walk, twelve
ordinary east/west land steps reduced the balance to 360. A south shortcut
did not move the hero. Two clicks on the visible opposite shore at `(15,2)`
showed no route and left the hero at `(15,34)` with 360 points and 92 mana.
The direct shortest route costs 400 points. This observation does not establish
whether the server rejected a movement request: the client may have declined
to issue one.

As a positive control, the scenario was restarted, Water Walk was cast again,
and the same visible shore was selected. The interface displayed the route,
and a second click moved the hero `(15,34) → (15,35) → (15,0) → (15,1) →
(15,2)` with 1,160 points remaining. The new client session appended records
0–47 to `runtime/observation-command-test.jsonl` under the TB5 build root;
the movement counts and mana are taken from those observations. This checks
one GUI route and its positive control, not general recovery or fog safety.

The same comparison was repeated with Flight on the next local client build,
`8552be9` plus the scenario-list coordinate fix. After twelve land steps,
selecting the shore with 360 movement points produced no route and left the
hero at `(15,34)` with 85 mana. After restarting and casting Flight with
1,560 points, the same destination produced a route and the hero crossed
`(15,34) → (15,35) → (15,0) → (15,1) → (15,2)`, ending with 1,160 points.
The new appended observation session records this at sequences 0–45. The
same limitations on attribution and generalization apply.

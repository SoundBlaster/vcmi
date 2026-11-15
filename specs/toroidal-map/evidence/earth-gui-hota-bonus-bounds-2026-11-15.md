# Earth GUI startup and HotA bonus bounds

On macOS arm64, a local SDL3/Metal client loaded the exploratory 252×576
Earth draft with the HotA port enabled. The source map and its derivative
remain local on TB5. The test temporarily linked the HotA mod and archives
from the user's installation into the local runtime, added the draft under a
temporary map name, and selected a temporary mod preset. After the client and
server exited, it restored the prior mod settings and removed those links; the
restored settings matched the backup's SHA-256.

The first launch used `cf01289`, the local HotA port at `6a5e21f` and the
draft with SHA-256
`58e7b7e6bf94c117d350028757c9cf1c7160bf3107fe435cf003306d3dbd74a6`.
It loaded the map and emitted a graphical map observation with dimensions
252×576 and toroidal mode, but the HotA definitions caused schema validation
and parsing errors for `AT_MOST` and `AT_LEAST`. Affected logged entries
included Artillery, Ballistics, First Aid, Horn of the Abyss, Trailblazer and
Factory creatures. The existing parser fell back to an additive value for
these unrecognized strings, so successful startup did not establish correct
rules.

Commit `9ce0ab9` accepts the two names as aliases of the existing
`INDEPENDENT_MIN` and `INDEPENDENT_MAX` value types, respectively. It keeps
the existing enum and JSON serialization names: a parsed alias serializes
back to its legacy spelling. Tests exercise schema validation, parsing,
legacy-name serialization and rereading, and the existing bounded calculation
with additive bonuses. A lone bound retains the pre-existing standalone-value
behavior; the change does not redefine the bonus calculation.

The second graphical launch used a build based on `cf01289` with the source
changes committed as `9ce0ab9`, the same draft and a fresh validation cache.
It again reached a 252×576 toroidal map observation. The stdout log contained
no invalid-value errors or mod-loading issue messages in this short run;
it did contain a libpng interlace warning. Nullkiller started the red turn,
built taverns, recruited heroes and selected movement tasks. The process was
then stopped by the test; a full turn and normal shutdown were not observed.
The observation records state and renderer geometry, not visual continuity,
texture correctness, a complete playthrough or all mod interactions.

The local `BonusMigrationTest.*:BonusSystem*` run passed 11 tests. The full
`vcmitest` run after the parser change passed 2,003 tests, skipped eight
optional external scenarios and listed three disabled tests. Its XML and log
are `bonus-bounds-suite.xml` and `bonus-bounds-suite.log` under the TB5 Earth
research runtime. The red-player server-turn test with the HotA profile also
passed three repetitions for seed 1 (handoff) and seed 3 (battle start).

The first and second GUI logs are in `earth-gui-hi8nvyod/` and
`earth-gui-fixed-oh0nj2dm/` beneath that runtime. Their stdout SHA-256 values
are `475c97b7eeafc29412689be31f5383a2927544a4cd7fc341ca05fe3c29c60acd`
and `d37a8e9e1605a144ef9170c92e8c12a5ca986fdefeee5f0220d23b1ba00b4032`;
the second observation stream SHA-256 is
`ee007feca13d8cade97893e95c7ff2de4b143c33fe4d48e34882caed3021e1b8`.
Each launch used `--testmap Maps/toroid-earth-probe.vmap --testseed 1
--spectate --oneGoodAI --spectate-skip-battle --spectate-skip-battle-result
--disable-video`, a local server port, and `VCMI_MAP_OBSERVATION_PATH`.

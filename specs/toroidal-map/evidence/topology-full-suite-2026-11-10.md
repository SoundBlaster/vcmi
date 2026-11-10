# Full test suite with external compatibility fixtures

On 2026-11-10, the isolated macOS arm64 TB5 build compiled the test sources
from commit `190915d`; the generated version string still identified its
parent `f97bf21` because compilation preceded that documentation/test commit.
The executable ran the entire enabled `vcmitest` suite: 1,466 tests from
226 suites passed, with no skips and four disabled tests. The complete log is
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/vcmi-full-suite-190915d-with-external-fixtures-2026-11-10.log`.

The run supplied three optional read-only inputs: the old untagged laboratory
VMAP, an old untagged laboratory save, and the Complete map `Dead and Buried`.
The compatibility tests decoded the prototype files and round-tripped the
legacy map. A separate invocation of the same legacy-map test passed for the
H3 demo `h3demo.h3m`. The four inputs had the same SHA-256 hashes before and
after these runs as their previously recorded evidence files.

The four disabled tests include the unresolved raw network-codec layout
mismatch regression. The suite does not prove graphical presentation,
network confidentiality or support for every map and AI profile. This is a
local macOS run, not upstream CI.

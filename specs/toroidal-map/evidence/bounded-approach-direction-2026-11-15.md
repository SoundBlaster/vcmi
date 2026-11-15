# Server direction check on bounded maps

The guarded-mine comparison exposed a legacy difference: the bounded-map
pathfinder rejected direct entry from the mine's forbidden side, while the
server accepted an equivalent direct request. The initial bounded controls
failed the rejection and state-preservation assertions before the fix;
`runtime/guarded-mine-directions-2026-11-15.xml` records that run.

The server now applies its existing destination approach-direction check to
standard LAND, SAIL and WATER requests on bounded maps as well as toroidal
ones. It still uses the map's own topology to calculate direction. The
bounded-map source-side check and other movement restrictions were not
changed. This intentionally changes direct server requests that previously
bypassed the ordinary client's pathfinder; it does not claim every bounded
server movement rule agrees with that pathfinder.

The updated mine fixture checks rejection without moving, spending points,
changing ownership or opening a visit, alongside allowed approaches, the
dialog, forced victory and server-handled retreat. Two forbidden directions
use synthetic start positions covered by the mine; see
the [fixture record](guarded-mine-directions-2026-11-15.md) for exact limits.
The focused 67 tests passed (`runtime/bounded-direction-fix-2026-11-15.xml`).
The full macOS arm64 local suite with the prototype VMAP, prototype save and
Complete legacy H3M supplied passed 1,854 tests; three tests were disabled.
Its binary was built from `174ab82` plus the code committed as `ffff640`;
reports are `runtime/full-suite-bounded-direction-2026-11-15.{log,xml}` under
the TB5 build root. The build reported duplicate-library and missing Conan
search-path warnings. No graphical or remote multiplayer session was run.

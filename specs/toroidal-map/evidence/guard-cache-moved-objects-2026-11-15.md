# Guard cache after moving an object

At `b88b19b`, a synthetic server test used `CGameHandler::changeObjPos` to
move a monster from `(35,35,0)` to `(10,10,0)`. On both bounded and toroidal
maps, the cached guard still pointed to the vacated position and did not
recognize the new position. On the torus the stale entry also affected
`(0,0,0)` across both seams. The two original regression cases failed before
the fix. This uses a general game-event API; no ordinary in-tree gameplay
caller that moves monsters was identified during this check.

`GameStatePackVisitor::visitChangeObjPos` now refreshes the guard cache around
both old and new object footprints after moving the object. The refresh uses
the same bounded or topology-aware area calculation already used for adding
and removing objects. It applies to every moved object because blocking
visitable objects can also affect guard accessibility. Ordinary hero movement
uses a different packet; Summon Boat uses `ChangeObjPos`.

The extended test runs on both topologies and with the destination on the
same or another level. It serializes the move packet into two independent
in-memory game-state replicas, and checks their old and new guard positions
against the server state. These are serialized replicas, not a TCP or GUI
session. The four cases, existing guard removal, direct monster and hero
attacks, and Summon Boat tests passed three consecutive runs: 45 tests each
time. `vcmitest`, `vcmiclient` and `vcmiserver` built on macOS arm64; the
linker reported existing duplicate-library and missing search-path warnings.
The complete local suite passed 1,610 tests without skips, with four disabled;
the TB5 log is `runtime/full-suite-moved-guard-2026-11-15.log` under the VCMI
build root. It included the old prototype map and save and the Complete
`Dead and Buried` H3M. Other object shapes, mod scripts, and remote clients
remain unverified.

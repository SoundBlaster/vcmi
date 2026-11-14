# Emergency relocation to known land

For toroidal maps, an on-foot AIR or WATER hero who cannot land in place or
reach a safe ordinary landing can be returned to a valid recorded departure
tile. If that origin is missing or unsuitable, the server can now relocate the
hero to known safe land on the same level. This changes gameplay in that
exceptional recovery case: the hero keeps the army and artifacts, spends all
remaining movement, and the owner receives a message. The relocation does not
invoke departure or arrival object visits.

The destination must already be visible, be passable, unblocked land, have no
hole or guard, have no visitable objects other than events, and have a fully
revealed guard neighborhood and hero sight range. Thus the relocation does not
intentionally open fog or trigger a hidden event. The implementation searches
by squared wrapped horizontal distance with deterministic coordinate tie breaks;
it prefers a valid recorded origin and attempts to preserve other heroes' origins.
The result is not necessarily the closest tile when multiple heroes need help.

All heroes' destinations are chosen before any recovery is applied. Heroes
with larger sight ranges are assigned first; origins can be reassigned to make a
group recovery possible. This remains a constrained fallback, not a proof that
every possible state can be recovered. If no suitable known tile exists, or a
transient hero is in a boat, this fallback does not invent a destination or
complete the recovery. Other dynamic-world cases and global optimality of group
assignment are not established.

## Evidence and scope

- `OldSaveCanRecoverToNearestKnownSafeLand` loads a previous server-state
  version through an in-memory archive, then checks wrapped nearest-land
  selection, movement, army and spellbook. It is not an on-disk save test.
- The `EmergencyRelocation*` and `EmergencyRecovery*` cases exercise hidden
  land, an occupied candidate, recorded-origin preference, multiple heroes
  with differing sight ranges and origins, and a synthetic 252×252 map.
  The large-map case took 22–28 ms in these local runs; this is not a general
  performance benchmark.
- `TransientRecoveryMustNotReturnHeroToAnotherLevel` checks a synthetic stale
  cross-level origin and no same-level land, including a memory archive round
  trip. A suitable same-level fallback can be used when one exists.
- `TcpLobbyStartsToroidalGameForTwoPeers` constructs a stranded hero with no
  recorded origin by applying a synthetic movement packet, then sends a real
  end-turn request over loopback TCP. It checks the resulting relocation,
  movement layer, points, turn transfer and fog agreement on both replicas.
  This does not demonstrate network loading of an old save or remote play.

Actual GUI play, live timing, broader object configurations, and recovery with
no known safe tile remain unverified or unresolved.

The local full suite passed 1,512 tests from 226 suites; three tests requiring
external fixtures were skipped and four tests remain disabled. Log:
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/full-suite-emergency-known-land-2026-11-14.log`.

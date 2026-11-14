# Group recovery from transient movement

A manual end-turn request can now recover multiple on-foot AIR/WATER heroes
when each can safely land in place or return to its recorded departure tile.
Before moving anyone, the server validates every plan, including distinct
return tiles, occupancy, visibility and the other constraints described in the
[origin-return record](topology-transient-origin-return-2026-11-14.md). If one
plan fails validation, no recovery packet is sent and no hero is moved.

For an accepted group, a single movement activity spans the whole batch so
normal activity settlement and its victory checks cannot run between those
moves. After settlement the end-turn handler rechecks whether the player is
still acting and in game before ending the turn. The timer can still land an
individual hero through an ordinary known route first, then retry the group.
This prevalidation does not claim transaction rollback for unexpected errors.

`HeroMovementTest.MultipleTransientHeroesRecoverOnlyWhenTheWholePlanIsSafe`
uses two water walkers with artificially reduced points and a third hero
synthetically placed in AIR above clear land. An occupied origin prevents any
movement, including the in-place landing. After the obstacle is removed, all
three reach LAND. The test records one movement-block start, three movement
packets, then one movement-block end. The single-hero origin-return and TCP
loopback tests also pass. This is a local synthetic fixture; it does not prove
all interactions, victory conditions, graphical clients or remote networks.

Recovery can still fail for old saves without an origin, inaccessible or
occupied origins, vehicles and other cases. G12 remains pending.

On macOS arm64, `vcmitest` and `vcmiserver` built. The full suite with the
prototype save, VMAP and legacy H3M fixtures passed 1,498 tests from 226
suites without skips; four remain disabled. The log is
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/full-suite-multi-hero-recovery-2026-11-14.log`.

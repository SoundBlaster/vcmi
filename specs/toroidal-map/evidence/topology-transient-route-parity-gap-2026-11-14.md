# Client/server transit route disagreement and bounded repair

The original `HeroMovementTest.DISABLED_TransientClientServerRouteAgreement`
reproduced a defect. On a completely revealed 36×36 toroidal map
with a single land tile, an on-foot hero with Water Walk and 1,200 movement
points has a pathfinder route to the seventh adjacent water tile in turn zero.
On executing the corresponding seven direct server movement requests, the
server accepts the first six and rejects the seventh because the hero would
no longer have a known landing reachable with the remaining points. The
experiment was run with the test enabled and failed on the seventh step.

The enabled `TransientClientServerRouteAgreement` now checks that the raw
search still sees the tile, but `getPath` and `getPathInfo` reject it; the
six-step route is available and succeeds on the server. It checks invalidation
after movement points change and eight concurrent readers. A second test covers
Flight crossing a horizontal seam. `VisibleShoreRemainsReachableNearFog`
executes the pathfinder's returned nodes through the server to an actual shore.
Two replicas in the loopback TCP lobby also reject the unsafe AIR path in a
synthetic fog state, after which the server rejects the transmitted request.
Paired hidden-terrain and hidden-guard tests check two non-disclosure cases.

The shared landing search is evaluated lazily for a requested path or cursor
node and cached for that search. Internal `getNode` remains the raw graph:
the landing search uses it to avoid recursion. Validation covers the current
turn's uninterrupted prefix, and stops predicting after interactive actions or
turn changes. Dynamic fog revealed during travel can change later decisions;
the server remains authoritative. No exhaustive route-parity, AI, information
security, or remote-network claim follows from these fixtures.

On this Mac, a 252×252 mostly-water fixture that builds and validates several
nearby routes took approximately 38–40 ms in local Google Test runs. This is
one scenario, not a general benchmark.

After rebuilding the client, server and test executable, the full local suite
ran 1,518 tests from 226 suites: 1,515 passed and three external-fixture tests
were skipped; four other tests remain disabled. The route-agreement reproducer
is enabled. The log is
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/full-suite-transient-route-parity-final-2026-11-14.log`.

## Interactive attempt

A separately development-signed client and a separate toroidal copy of the
single-island Flight/Water Walk map were prepared on TB5. The new client process
started but Computer Use could not obtain its window. A process sample showed
the main thread waiting in `open` while opening its default log. A second
signed client with a temporary local log got past logging, then its log stopped
at the `Saves` scan; a process sample showed a directory open waiting. The
`Saves` path resolves through the existing user-data symlink to TB5. The
filtered TCC query for the first process did not show a removable-volume
prompt; the cause of the wait is not established. Neither attempt reached an
observable game window, so no graphical or interactive route result is claimed.

### Subsequent macOS launch

After the user approved launching the game, the local-log client opened its
window, loaded the map list, and started an ordinary bundled scenario through
to its adventure screen. The log also records the test map's header being
loaded while scanning the scenario list. This confirms startup and a basic
ordinary-map launch, not the toroidal Flight or Water Walk behavior. Attempts
to focus the scenario filter through Computer Use did not succeed; the
prepared toroidal map and its routes have not yet been exercised in the GUI.

The window title identifies this separately packaged client as `360ffdf`.
It includes the route-filtering work prepared before `506fc7b`, but was
packaged before that commit's final cache synchronization change. It is not
evidence that the exact final committed binary was interactively tested.

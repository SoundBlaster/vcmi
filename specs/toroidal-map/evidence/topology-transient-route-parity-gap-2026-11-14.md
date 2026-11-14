# Known client/server transit route disagreement

`HeroMovementTest.DISABLED_TransientClientServerRouteAgreement` is a
reproducer for a known defect. On a completely revealed 36×36 toroidal map
with a single land tile, an on-foot hero with Water Walk and 1,200 movement
points has a pathfinder route to the seventh adjacent water tile in turn zero.
On executing the corresponding seven direct server movement requests, the
server accepts the first six and rejects the seventh because the hero would
no longer have a known landing reachable with the remaining points. The
experiment was run with the test enabled and failed on the seventh step as
expected for the defect; the test is disabled until parity is implemented.

This establishes disagreement between local pathfinding and the authoritative
server for this synthetic layout. It does not establish exactly how the GUI
draws or responds to the route, nor does it test Flight, remote clients, or
every alternative route. A repair needs shared movement-safety semantics while
avoiding recursive pathfinder use and preserving legal paths to visible shores;
performance and fog-of-war behavior must be measured as part of the repair.

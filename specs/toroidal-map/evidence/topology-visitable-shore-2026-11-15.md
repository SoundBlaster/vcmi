# Visitable shore across seams

On macOS arm64, the test binary built from `d53c2a2` with the new test
committed as `d433f70` passed 48 cases in
`EdgeAndCornerOnBothLevels/ToroidalGuardedDisembarkTest.*`. Sixteen new cases
stage a boat on each outward direction of an edge or corner, on each of the
two levels. A synthetic visitable but nonblocking event occupies the landing
tile. The test checks that the pathfinder does not choose a direct landing,
the server rejects the move, the hero and boat remain together at sea, and
movement points and the event remain unchanged. After the event is hidden,
the server accepts disembarkation and the same boat remains on the water.

The other 32 cases cover the previously implemented guarded-shore victory and
retreat scenarios. This test does not resolve the separate hidden-event and
Flight landing issue, or verify graphical, network or other object behaviors.
The XML output is on TB5 at `runtime/shore-visitable-seam-2026-11-15.xml`,
SHA-256 `941f27e41102d7844edd706543bd2ee062c7861e0083f49f8907a3d72b43a1b6`.

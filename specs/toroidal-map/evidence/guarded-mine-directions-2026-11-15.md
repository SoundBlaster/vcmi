# Guarded mine directions and retreat

`ToroidalGuardedMineTest` places a guarded neutral mine at the destination of
each of eight seam directions, on each of two levels. Matching bounded-map
controls use the same mine and direction in the map interior. It checks
whether the ordinary pathfinder offers the direct step, and checks the
toroidal server's direct response. For offered steps it checks the charged
movement points, refusal of the mine dialog, a revisit, the defender identity,
and either forced victory or a server-handled attacker retreat. Victory
captures the mine; retreat removes the hero and leaves the mine neutral. Both
outcomes close the object visit.

The mine footprint blocks the constructed starting cell in four of the eight
directions. Those start positions are synthetic and should not be interpreted
as legal routes into the mine. For the three northward directions the start is
free of this mine and the direct step, dialog, victory and retreat were
exercised on the seam and in the interior. The southeast start is free of this
mine, but the pathfinder does not offer that direct approach and the toroidal
server rejects it without charging movement. Object-specific approach masks,
not seam direction alone, determine availability.

An initial bounded-control experiment directly submitted the disallowed
approaches and found that the bounded server accepted them even though its
pathfinder does not offer them. The toroidal server rejects these requests.
The initial observations are recorded in
`runtime/guarded-mine-directions-2026-11-15.xml` on the TB5 build root.
The initial passing regression in `b3c071f` avoided issuing those bounded
requests; the subsequent [bounded direction fix](bounded-approach-direction-2026-11-15.md)
extends the same assertions to them.

On macOS arm64 the final run passed 67 tests, including 64 parameterized
cases and three existing mine regressions. The test binary was built from
`45c0fa8` plus the tests in `b3c071f`; artifacts are
`runtime/guarded-mine-origin-2026-11-15.{log,xml}`. The build reported
duplicate-library and missing Conan search-path warnings. This shared-state
fixture does not test the battle UI, a naturally played battle, remote clients,
or all mine types and surrounding objects.

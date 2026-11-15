# Selective object approaches across seams

The documented JSON `visitableFrom` mask uses `+` for an allowed direction
and `-` for a forbidden one. `ObjectTemplate::readJson` previously added `+`
bits to the constructor's default directions instead of replacing them, and
could retain bits from an earlier JSON read. The loader now clears the mask
before applying the JSON. Missing or empty masks remain non-visitable. The
separate H3M binary reader was not changed.

This corrects JSON templates on both bounded and toroidal maps. Maps and mods
that relied on directions implicitly enabled by the old behavior must declare
those directions explicitly with `+`. The earlier seam entrance fixture was
updated to specify the direction it actually needs.

Before the fix, all 16 selective-approach seam cases failed while checking the
loaded mask. Afterward, all 16 passed for eight edge and corner directions on
each of two levels. For a gold resource, each case checks that the pathfinder
does not offer a direct forbidden step and that the in-process server rejects
it without changing the hero, movement points, resource or gold; it then checks
an allowed direct visit and collection. An alternate route may still exist.
Repeated JSON reads and serialization round trips were checked separately.
Twenty focused tests passed together; the complete macOS arm64 suite passed
1,709 tests with three disabled. Logs and XML results are in the TB5 VCMI
build root under `runtime/directional-entrance-*-2026-11-15.xml` and
`runtime/full-suite-directional-json-2026-11-15.*`. Build warnings included
duplicate libraries and missing Conan search paths.

A graphical client and server built from `68e6b3b` plus this uncommitted fix
started the road-axes demo using the installed game resources. Its observation
stream recorded 1,000 gold collected from the corner, followed by a move from
`(0,0,0)` to `(35,35,0)`. The client exited normally. Logs are in
`runtime/gui-directional-json-2026-11-15/` on TB5. This smoke test does not
exercise a selective JSON mask visually, prove behavior for all object types or
mods, or establish animation continuity.

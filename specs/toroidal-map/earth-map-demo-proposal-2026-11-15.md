# Candidate Earth map demonstration

This is an exploratory proposal, not an accepted design or evidence that the
map has been converted. The user proposed adapting [Earth Rules the World
2.0](https://h3maps.com/maps/2-0-8381/) by jabba1000. The listing describes
a 252×252 map with two levels and requires HotA 1.7.2 or newer. Its two level
previews depict parts of the world, including the Americas on the underground
level. The user's sketch places the Arctic between the original surface and
an inverted Americas section, with Antarctica meeting across the top and
bottom seam.

VCMI supports rectangular maps, and our branch has separately tested 36×23
and 23×36 toroidal maps; see [rectangular evidence](evidence/topology-rectangular-vmap-2026-11-15.md).
Stacking two complete 252×252 levels would yield a 252×504 single-level
starting canvas with the same number of tiles, but geography changes could
require other dimensions. Loading, performance and playability at that size
are not established by the smaller tests.

The editor can copy selected objects between levels, but its current copy
path does not copy a terrain region or provide a general region rotation.
A conversion tool could first assemble terrain, then transform object positions
without blindly rotating their sprites and interaction masks. It must handle
references, events, portals, terrain transitions, roads, rivers, seam-crossing
footprints, collisions and gameplay balance. Seam-adjacent tiles meet at the
same x coordinate on the top and bottom edges; crossings do not rotate or
mirror a hero. Art and connectivity along both seams need deliberate review.

The downloaded H3M is kept outside the repository on TB5 and has SHA-256
`767e28f90b79fceb08be5a93d3e3c9c4d4bf7ab9e6dac9fd95c91806de387ff5`.
Its header identifies HotA format version 7. In the core+vcmi+vcmi-test
environment, the original loader threw `vector` while reading the HotA
predefined-hero extension: the table was larger than the installed hero pool.
After handling default entries beyond that pool, loading proceeds further,
but fails during water-content calculation because terrain index 10 is not
available in that test environment. The test environment only enables a
minimal HotA-format fixture, not the HotA content mod. The user's Complete
installation on TB5 does contain original HotA resource files; their presence
alone does not establish that the VCMI HotA port is installed or active.
These observations do not establish whether a fully configured VCMI can
load or play this particular map.

The loader change is covered by synthetic HotA v5 maps: default hero entries
past the installed pool load, custom entries for an unavailable hero report
its index, and an available customised hero is still created. The local full
suite passed 1,995 tests, skipped five optional tests and reported three
disabled tests. Its XML and log are under
`runtime/earth-map-research-2026-11-15/` on TB5. This suite uses the test
fixture mod; it does not demonstrate a full HotA installation.
The listing does not establish permission to redistribute the map or a
derivative; investigate the author's terms before publishing either.

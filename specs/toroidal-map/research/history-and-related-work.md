# Historical context and related work

This note records the [context supplied by the user](user-context-2026-11-14.md)
and a bounded source check on 2026-11-14. It is research context, not a
specification, implementation claim, exhaustive mod-catalog survey, or proof
that no other implementation exists. The current experimental implementation
and its evidence are tracked in the [implementation matrix](../implementation-matrix.md).

## Prior proposals

On 25 February 2010, Guitarzan proposed crossing the west/east and north/south
map edges in [Spherical Maps and Triple Level Maps](https://forum.vcmi.eu/t/spherical-maps-and-triple-level-maps/272).
On 26 February OnionKnight replied that wrapping was feasible but would need
substantial work. Tow_dragon also discussed pairing border tiles for more
varied surfaces and pointed out map-format and editor implications. The
thread also discusses a third level; that is a separate feature from the
present two-axis wrap. The thread alone does not establish participants' formal
roles at that time.

[VCMI issue #6469, “Cylindrical and toroidal topology for maps”](https://github.com/vcmi/vcmi/issues/6469)
was opened on 18 December 2025 and was open when checked on 14 November 2026.
A commenter linked an [earlier video](https://www.youtube.com/watch?v=n_4ITbUGm84)
and explicitly said they did not know whether its code remained available.
The video could not be inspected in this check; neither the availability of
its code nor equivalence with this branch is established. A GitHub search for
`toroidal` in upstream issues found #6469; this is not an exhaustive search
for related terms, repositories, pull requests or mods.

## Existing capabilities and related generator

The official [VCMI game mechanics documentation](https://vcmi.eu/players/Game_Mechanics/)
describes adventure-map zoom, rectangular and arbitrary map sizes, and removal
of the original map-object count limit. It does not list two-axis wrap as one
of those capabilities. Documentation is not a proof that no unlisted feature
or experimental implementation exists; “any size” is also subject to real
format, memory and performance limits.

[OmniMapGen](https://github.com/Renegade1993/OmniMapGen) is an external random
map generator for VCMI. Its README describes an add-on for the Dead Man’s
Boots fork as well as standalone command-line use; it writes `.vmap` files,
supports free layouts with chunked Wave Function Collapse and supports
RMG templates. The README inspected for this note does not claim toroidal
edge handling. This does not establish that its source has been fully audited
or that its generated maps are compatible with this experimental branch.
GitHub identifies the repository licence as [MIT](https://github.com/Renegade1993/OmniMapGen/blob/main/LICENSE);
VCMI is GPL-2.0-or-later. Review licensing and attribution before reusing code.

## Architectural implications

Pairing opposite edges on both axes gives a finite periodic tile grid with
torus topology, not an unbounded world that generates new chunks. The forum
thread uses “spherical” in its title; an ordinary two-axis wrap is a torus,
not a sphere. Arithmetic such as `x mod width` and `y mod height` is only one
piece: pathfinding and movement rules, AI knowledge and distance, rendering and
scrolling, sight and guards, spells, persistence and network agreement must
also use compatible topology. Existing bounded maps require an explicit
compatibility policy. A toroidal map does not automatically have visually or
logically matching terrain, rivers, roads, and connections at its seams.

Random-map generation is a separate concern. A chunked generator or template
editor does not by itself make its borders periodic or preserve intended zone
connectivity across seams. See the branch's [template generator design](../template-generator-design.md)
for the current design candidate and the implementation matrix for its status.
The user's initial inference that an engine fork is a practical route is
consistent with this branch's cross-cutting changes; it is not a universal
proof that no modding interface could ever support such a feature.

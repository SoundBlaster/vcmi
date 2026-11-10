# Map topology and legacy content

This is an explanation of the current pilot, not a claim that every gameplay
system or compatibility profile is complete. The [implementation matrix](implementation-matrix.md)
and [coordinate audit](coordinate-audit.md) track verified behavior and gaps.

## Why topology belongs to a map

The same layout has different gameplay when its edges become neighbors: a
hero can take a shorter route, an enemy may be adjacent, a spell's target may
be in range, and a gate may be paired differently. Those changes need to be
consistent for every participant and after save/reload. The pilot therefore
stores the choice in `CMapHeader::toroidal`, rather than treating it as a
local camera preference. Both map levels use the same choice, but wrapping
never changes the level.

Existing `.h3m` maps have no declaration for this new topology, and legacy
content without the field remains bounded. The editor can open `.h3m` and
`.vmap` files and has a topology control; its Save As format is `.vmap`.
New toroidal `.vmap` headers use the branch-local integer major version
`0x54520004` and `topology: "toroidal"`. This marker makes newly written
maps distinguishable from an unrelated future ordinary version 4. The reader
also accepts this branch's earlier untagged version-4 prototypes; their
origin cannot be established from their version alone. A missing topology on
older format versions means bounded behavior. These are pilot format choices,
not upstream-approved allocations. The actual editor workflow and full
legacy gameplay coverage are still being verified.

A session-wide override for any old map is possible in principle, but would
change that map's rules and needs an explicit choice shared by the server,
clients and saves. The current implementation does not offer such an
override. Conversion also requires checking terrain transitions, roads,
passability, scripts and object placement at the joined edges. The pilot can
now save and load static objects that cross a seam, with limits described below.

## Agreed legacy footprint policy

When the author explicitly converts a legacy map to toroidal topology, object
footprint cells outside the old rectangle should appear at their corresponding
same-level tiles across the seam, in both axes and at corners. This applies to
visible, blocking and visitable cells. Overlap with empty terrain or a
non-interactive obstacle is not by itself a reason to discard an object or its
mask. The [demo-map diagnostic](evidence/topology-legacy-editor-2026-11-10.md)
shows why this matters: many legacy objects already have outside footprints.

VCMI already has per-cell drawing layers computed from each object's mask and
an object `printPriority`; the branch adapts part of the ordering to wrapped
coordinates. A tile also keeps separate blocking and visitable object lists.
The server's `objectVisitedBy` selects the last visitable object, excluding a
hero on top. Drawing order, movement passability and interaction selection
are distinct mechanisms: their agreement on newly overlapping interactive
objects must be investigated and tested. The editor now warns about multiple
static visitable objects sharing a tile when one crosses a seam. This warning
does not resolve selection or establish a general interactive-layer policy.

The pilot rejects static footprints wider or taller than the map period, since
their repeated image slices are not supported. The wrapped-footprint work does
not turn legacy maps into toroidal maps unless an author explicitly opts in.

The initial demonstration is experimental and uses maps adapted to the
topology, without ambiguous interactive-object overlaps at the seams. General
conversion of arbitrary legacy content and resolution of those overlaps are
follow-on compatibility work, not prerequisites for demonstrating such a map.
The agreed footprint-wrapping policy remains the target for that work; complete
visual and gameplay compatibility is not yet established.

## What the shared layer does

`CMapHeader::getTopology()` constructs a `MapTopology` from map dimensions,
level count and the flag. It offers explicit operations for canonicalizing a
position, calculating shortest displacement, comparing horizontal distance
when a caller deliberately ignores level differences, and enumerating wrapped
areas without duplicate tiles. For example, on a 36×36 torus `(0, 0, 0)` and
`(35, 35, 0)` are one diagonal step apart; on a bounded map they are not.

This is a shared policy layer rather than an automatic interception of every
coordinate expression. A tile getter cannot repair a move already rejected
by an earlier bounds check, an AI heuristic that subtracts coordinates, or a
renderer that must display repeated images on both sides of the viewport.
Those consumers have to call the topology operations at the appropriate
boundary. A missing coordinate such as `(-1, -1, -1)` must be recognized as
a sentinel before any wrapping, otherwise it could become a real map tile.

The audit is deliberately a candidate inventory, not proof that every match
is a defect or that every call site has been found. Changes are accompanied
by bounded-map regression checks and separately recorded runtime evidence.
The current numeric serialization guard is not a unique capability identity;
multiplayer and interchange with independently evolved VCMI versions remain
unverified.

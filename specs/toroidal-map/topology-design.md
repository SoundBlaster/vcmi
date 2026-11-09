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
In this branch a toroidal `.vmap` header uses `versionMajor: 4` and
`topology: "toroidal"`. A missing topology on older format versions means
bounded behavior. These are local pilot format choices, not upstream-approved
format allocations. The actual editor workflow and full legacy gameplay
coverage are still being verified.

A session-wide override for any old map is possible in principle, but would
change that map's rules and needs an explicit choice shared by the server,
clients and saves. The current implementation does not offer such an
override. Conversion also requires checking terrain transitions, roads,
passability, scripts and object placement at the joined edges. In particular,
the pilot currently rejects static objects whose visible, blocking or
visitable footprint crosses a seam.

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

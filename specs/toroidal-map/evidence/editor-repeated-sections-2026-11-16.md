# Editor repeated viewport sections

The editor's topology helper now splits a displayed rectangle into sections
whose coordinates map directly to canonical coordinates in one map period.
Sections retain both positions so a renderer can draw the same map data at
each displayed copy. The helper is not yet connected to editor rendering or
input; it does not enable scrolling by itself.

The focused test independently enumerates every displayed cell and compares
its mapped coordinate with `MapTopology::canonicalize`, including both
seams, a corner, negative positions and more than one period. On macOS arm64,
`vcmieditor` and `vcmitest` built and this test and the existing minimap
viewport test passed. Existing linker search-path warnings remained.

Remaining work includes integrating repeated sections into every visible
layer and invalidation path, recentering scrolling without a visible jump,
mapping input and gestures to canonical cells, preserving bounded behavior,
and exercising actual graphical interactions.

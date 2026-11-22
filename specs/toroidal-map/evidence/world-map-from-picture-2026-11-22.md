# World map generated from the landing page picture

[`tools/prepare_world_map.sh`](../tools/prepare_world_map.sh) builds a
256×128 toroidal VMAP from `deploy/lobby/site/world.jpg`. Python scripts
classify the picture into terrain letters, smooth the grid with wrapping on
both axes, and mark mountain and forest tiles for obstacles. The explicitly
enabled `MapManager.PaintWorldMapProbe` test then draws the terrain with
`CMapEditManager`, places towns from
[`tools/world_map_sites.txt`](../tools/world_map_sites.txt), mines, guards,
treasure, shipyards with boats, whirlpools and landmarks, and fills the
marked tiles through `EditorObstaclePlacer`. It runs with only the `vcmi`
and `core` mods, so the map references no test fixtures or HotA content.

Terrain drawing changes about 3.5% of the grid tiles, mostly along coasts,
where transition patterns expand a terrain. Before obstacles are placed,
paths are reserved between towns of one landmass and from every visitable
land object to a town or a shore. In the recorded run every such object had
a path; Eurasia and Africa form one landmass with five starting towns, while
the Americas and Australia are reached by sea. This checks static occupancy
only, not guard zones, hero movement or AI behavior.

Grid, mask and terrain are identical across runs. Obstacle instance names
differ between runs because the placer inserts its set of objects in pointer
order; the object type counts were equal. The map reloaded as toroidal in the
test and opened in the editor on macOS arm64. Neither the editor validator
nor a game on this map was run.

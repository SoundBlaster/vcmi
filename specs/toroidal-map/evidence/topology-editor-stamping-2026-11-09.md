# Editor object stamping groundwork

Editor object stamping, image slicing and reordering now use canonical tile
positions and object-local wrapped coordinates. Restamping examines all
objects on toroidal maps so an object whose anchor is on the opposite side
is not excluded by the bounded-map rectangle optimization. The static
seam-footprint restriction remains enabled; object interaction and topology
switching are separate concerns.

On 2026-11-09 on macOS arm64 with copied Complete assets, `vcmieditor` built
successfully (`editor-stamp-build.log`). A temporary TB5 `.app` using the
built executable opened a separate copy of `toroidal-demo.vmap`. In the GUI,
terrain, buildings, object markers and minimap were visible. The editor
closed normally without saving. Source and test-copy map SHA-256 matched:
`c08c08bd577654b31415855efa3a6eccaf3bf4de65485a767741d3ffdf48d4e3`.
The screenshot was observed through UI automation and was not saved to a
file. This map has no seam-spanning static objects; the observation does not
verify seam editing, brush operation, saving, or sprite ordering at a seam.

Source baseline `79d5c4b` with local change:

```text
0dde76c5d9b02e6ba30aa9f8a9a54a2744390e5252044b05ef965bf16427e804  mapeditor/maphandler.cpp
```

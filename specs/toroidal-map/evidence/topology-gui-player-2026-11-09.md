# Toroidal viewport: human-player observation

On 2026-11-09, a human-player session was started through the ordinary macOS
client UI using Computer Use. The window identified the client as
`1.8.0.768833d`; the experimental app uses build resources and shared libraries
from the TB5 build tree, so the window label alone is not an exact identity for
all loaded code. The server log identified the selected map as
`MAPS/toroidal-demo` at 22:26:55. The local map file had SHA-256
`c08c08bd577654b31415855efa3a6eccaf3bf4de65485a767741d3ffdf48d4e3`.
The map and copied Complete assets are described in the
[spectator record](topology-gui-spectator-2026-11-09.md).

The player controlled red, with Valeska and a town visible. Unlike the earlier
spectator run, unexplored terrain was obscured. Using `Ctrl+Left` and `Ctrl+Up`
moved the camera towards the west and north boundaries; the radar displayed
split portions at the map boundary. Two screenshots saved using the game's F3
command are retained outside this repository, under the runtime user-data
directory on TB5:

```text
54678354495cee112030ff80cc4f42a8180d8fa9fa2d312ff6df661622718ca8  extracted/screenshots/screenshot-20261109T222749.png
4d7b85c1d0b1dc7e9c72a26223d25df0e90da221d1786ad10fb471dfc4c52564  extracted/screenshots/screenshot-20261109T222951.png
```

These are discrete observations, not a continuous recording. They do not prove
smooth animation, visibility correctness for every cell, a hero crossing, map
selection from the minimap, or the other directions. The map's original art
was not authored to join at its boundaries. This observation supersedes only
the earlier record's unsuccessful attempt to enter a human session; all other
limitations and pending acceptance criteria remain.

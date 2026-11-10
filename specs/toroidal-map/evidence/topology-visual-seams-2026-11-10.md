# Visual seam fixture: road, river and terrain

`MapFormat.ToroidalVisualSeamFixtureRoundTrips` creates a separate 36×36
variation of the adapted laboratory map. With a fixed random seed, the map
editor's terrain, road and river operations paint a dirt patch and a dirt
road across the horizontal seam and a water river across the vertical seam.
It saves and reloads a toroidal VMAP and checks the terrain and line types
and selected direction/flip fields on the adjacent edge tiles. The existing
gameplay fixture is unchanged. Setting `VCMI_TOROID_VISUAL_OUTPUT` writes the
archive to a new path; the test refuses to overwrite an existing file.

On macOS arm64, a Release `vcmitest` built from `74d21df` plus the fixture
change and this focused test passed. It produced
`runtime/complete-data/Maps/toroid-visual-seams.vmap` in the checkout's TB5
build root, SHA-256
`e07706987e8fe51658c26977b880fdfb05a95a6c565fe666e449a4a9c6a40957`.
The build reported duplicate-library and absent Conan search-path warnings;
it exited successfully. The archive contains no copied game artwork.
The new round-trip test, the original adapted fixture test, and the two
toroidal terrain and road/river brush tests also passed together (four tests).

The diagnostic client stamped `d1d5403` loaded that exact map from an
isolated temporary profile with local copies of the Complete archives. At
the initial corner position, Computer Use screenshots showed the road and
river as continuous lines through their respective map seams. The `vcmimap`
cheat revealed the map for an unsaved test session. After independent camera
scrolling west and north, and then east and south, the road and river retained
their shapes in the sampled positions. A later view showed the dirt patch as
one continuous area across the horizontal seam. The game exited without
saving; temporary data copies were removed and profile links and settings
were restored.

This observation covers one road type, one river type and one grass/dirt
transition in discrete screenshots. It does not establish all frame timings,
every artwork orientation, fog rendering during motion, gameplay cost, or
seam-spanning static objects. The ordinary user installation was not changed.

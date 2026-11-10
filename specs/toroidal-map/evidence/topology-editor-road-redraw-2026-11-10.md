# Editor road drawing and terrain redraw across seams

The editor draws the upper half of a road tile from the tile immediately
north. Previously the north edge always omitted that contribution, including
on a toroidal map. The editor now resolves that source with the map topology;
bounded maps still have no northern neighbor at the top edge.

After terrain, road or river edits, the editor redraws a rectangle around the
changed selection. Previously this rectangle did not reach the opposite edge
when a brush affected seam neighbors. On toroidal maps the same margin is now
split into canonical rectangles, including all four corner pieces when
necessary. The bounded redraw path retains its prior calculation.

`MapTopology.EditorRoadAndRedrawCrossToroidalSeams` checks road source
positions, level isolation, empty selections, edges, corners and a large
selection; it compares redraw coverage with independently enumerated wrapped
tiles and checks that the pieces do not overlap. Four focused topology,
terrain and road tests passed. The macOS arm64 editor and test targets built
successfully; the linker emitted the existing duplicate-library and missing
Conan search-path warnings. These checks exercise the extracted calculations,
not actual Qt painting or the editor GUI. A visual redraw check remains open.

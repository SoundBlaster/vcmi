# Editor clipboard and hero placement

The editor's clipboard previously checked the shifted anchor against the
bounded map rectangle and retained the original position if it was outside.
Pasting at a toroidal edge now canonicalizes the shifted anchor, including
both axes at a corner. Bounded maps retain the existing out-of-bounds
fallback. On paste, placement validation now sees the actual destination.
For drag-and-drop, hero placement validation also uses the destination that
will be committed, rather than the new object's pre-drag position. This makes
the existing prohibition on placing a hero at a main town's generated-hero
entrance apply to the actual destination, including wrapped coordinates.

The macOS arm64 `vcmieditor` and `vcmitest` targets built from the change on
TB5. The `MapTopology.Editor*` and `MapManager.*` filters passed 16 tests,
including clipboard coordinate checks at both corners, an ordinary bounded
paste, and bounded out-of-range fallback. These tests cover coordinate
calculation and the existing map-editing checks; they do not exercise real
editor clipboard or drag gestures or the main-town placement dialog. Those
interactive checks remain open.

Before this work, Computer Use identified the already open editor document as
`two-route-checked-seed-12345.vmap`, SHA-256
`95f9381bf4d5911414d4f897e80ee6dc2f9cdb807b0d317c5f26adef95c624f1`.
Its header has no toroidal topology marker. This is the bounded generated
sample, not the seed-3 toroidal sample used in the subsequent game. It was
not edited or replaced during this check.

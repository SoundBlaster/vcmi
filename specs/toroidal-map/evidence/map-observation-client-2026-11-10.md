# Observation trace in a running client

On macOS arm64 a development-signed client built from clean commit `879f71e`
was launched with `VCMI_MAP_OBSERVATION_PATH` in its application environment.
The 36×36 Toroid Seam Laboratory save was loaded from the isolated TB5 data
profile. Computer Use held Ctrl+Right and Ctrl+Down, saved an F3 screenshot,
and closed the client without saving the game.

The local JSONL file contains 1,800 parseable records from one session,
numbered consecutively from 0 through 1,799, with monotonically increasing
timestamps over 16.538 seconds. Every record reported `complete: true`.
There were 285 distinct camera centers. Two consecutive pairs crossed the
canonical pixel period of 1,152: `(1136,1136) -> (23,1136)` and
`(950,1149) -> (950,4)`. Every emitted displayed/canonical tile pair in
this trace agreed with modulo 36. The screenshot showed the red hero, nearby
resources, revealed terrain and fog after scrolling; candidate IDs 0, 2, 5
and 6 occurred in the trace. No candidate at the fixture's blue-hero or
blue-town reference cells `(18,18)` or `(27,27)` occurred. This is a narrow
check of those cells, not a complete fog confidentiality audit.

The trace records map show calls and renderer candidates, not presented
frames. In particular, the horizontal crossing above is a 39-pixel wrapped
camera change between two recorded calls. These measurements do not prove
that every frame was presented or that scrolling was visually smooth. The F3
screenshot is a separate observation, not synchronized to a particular
record. GUI widgets, semantic roles, input actions and an external protocol
are not yet implemented in this prototype.

Local artifacts remain on TB5:

- `runtime/map-observation-879f71e.jsonl`, SHA-256
  `8eb1e8b51a1bdba27581c8259e4567fd240b36766ef8056985797928d44eed7d`.
- `runtime/user-data/extracted/screenshots/screenshot-20261110T101741.png`,
  SHA-256 `c8138309f91c359a60cd56722f8d3b3288912fad71a2d9f3a0952eb7dea91bab`.
- Signed `runtime/VCMI Toroid Observe 879f71e.app`; client SHA-256
  `b3f0faef1c040ece43de371cd46b7ca59534e0b3edcc3fd66cec0c1a456c6064`.

The screenshot contains original game artwork and is not committed. Earlier
unit/build evidence and the opt-in interface are in
[the design note](../diagnostic-observation-design.md).

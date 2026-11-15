# Earth draft: active-object overlap audit

The read-only `tools/audit_earth_overlaps.py` compares serialized active
footprint cells in the original two-level converted VMAP and the final
single-level toroidal Earth draft. It wraps draft coordinates and subtracts
object pairs that already overlap an interactive cell in the bounded source.
It reports pairs with at least one visitable `A` or `T` cell. This is a mask
comparison, not engine pathfinding or proof that a player is blocked.

For source SHA-256
`69e1f1aff0398a7042e7b158b1ac638ae3237b23d0a5a8927f0afcb7ef0c4023`
and demo SHA-256
`61f9f042612fb86aa611c64e57b2c2e6ee7a39409cf2b4eeb1fd74b8b00d40b7`,
the audit reports 161 new interactive pairs; 267 active source cells lie
outside the bounded source levels and are excluded from the source baseline.
The JSON is local on TB5 at `runtime/earth-overlap-audit-2026-11-15.json`,
SHA-256 `85c06b6835517b647a964b92710466af27b6582fd7020990ed3ba81b67ea2d79`.

One candidate is `creatureBank_10789` and `creatureBank_11365`. At `(126,243)`
the first has `B` and the second `A`; at `(127,243)` these roles are
reversed. These are distinct source objects, and neither overlaps the other
in the bounded source under this calculation. The current draft places both
anchors at `(127,243)`. Actual visit behavior and an appropriate repair have
not yet been verified. Other pairs may represent intentional or harmless
overlaps; the count must not be interpreted as a confirmed defect count.

Three focused Python tests passed on macOS arm64, including seam wrapping,
pre-existing pair exclusion and noninteractive overlaps. The tool does not
test engine occupancy semantics, terrain, guards, movement, or map balance.

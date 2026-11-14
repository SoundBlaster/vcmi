# Upstream baseline: single island and transit movement

The game engine and pathfinder were built from upstream commit
`30e9207276f137658da5132705c05644bfe33aee`, before this branch's changes.
The accompanying [test-only patch](baseline-single-island-2026-11-14.patch)
adds two temporary tests to that checkout; it does not modify engine code.
The build and test output were placed on TB5.

## Automated experiment

Each test creates an ordinary bounded 36 × 36 map with one land tile at
`(18,18)` and water everywhere else. The hero has 1,200 movement points and
is given a **permanent test bonus**, either Water Walk or Flight. The entire
map is revealed. The tests passed on the baseline:

```text
HeroMovementTest.BaselineIslandWaterWalkProbe  PASS
HeroMovementTest.BaselineIslandFlightProbe     PASS
```

For both bonuses, the immediately adjacent water tile is reachable on the
corresponding WATER or AIR pathfinding layer. The default `AUTO` destination
lookup cannot build a path ending on that water tile. With direct server
`moveHero` calls, however, eleven consecutive eastward transit steps are
accepted; the hero ends at `(29,18)` with 100 movement points. The fixture
does not invoke a client, network transport, an actual spell cast or turn
transition. It does not demonstrate that a player can issue those eleven
steps through the normal GUI, nor what happens when a one-day spell expires.

The baseline client selects `AUTO` for ordinary map clicks unless the hero is
in an airship, and the default destination lookup chooses LAND or SAIL.
Its movement controller normally defers a requested stop while the current
path node is AIR or WATER. These are source observations, not a completed
interactive gameplay test; other interruption paths must be evaluated
separately.

## Interactive check pending

A separate baseline app and a bounded VMAP with one connected island were
prepared for a GUI check with actual Flight and Water Walk spells. At the
time of writing the app was waiting for macOS permission to access the TB5
removable volume. Its map loading, hero setup, interactive movement and
end-of-turn behavior have not yet been verified. No GUI outcome should be
inferred from the automated test above.

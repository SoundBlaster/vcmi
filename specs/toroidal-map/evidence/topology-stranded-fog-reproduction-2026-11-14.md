# Stranding through normal fog exploration

At commit `95bf789`, `HeroMovementTest.FogExplorationCanStrandWaterWalkerOnUnchangedMap`
constructs a 36 × 36 toroidal map containing a single land tile surrounded
by water. The hero starts on that tile with Water Walk and 1,200 movement
points. The map terrain does not change during the test, and fog is revealed
through normal server movement rather than being edited between steps.

Eleven accepted eastward WATER transit steps leave the hero with 100 movement
points, too far from the island to return this turn. An additional transit
step is rejected by the last-points guard; the existing automatic landing
cannot find a route. The hero remains in WATER transit. The known-landing
check treats an unexplored frontier as inconclusive, allowing exploration but
not guaranteeing a safe return. This is a deliberately documented failure,
not acceptance of stranded behavior.

The later [origin-return change](topology-transient-origin-return-2026-11-14.md)
addresses this particular sequence, and the test was updated to assert recovery.
G12 remains unsatisfied for cases without a usable recorded origin.

The original fixture establishes this server-side sequence on a synthetic map. It does
not demonstrate a full graphical playthrough, an actual timer transfer, or the
consequences of dismissing a hero. A solution still needs a recoverable,
deterministic outcome, without leaking unseen terrain or destroying the
hero's army and artifacts, and must address old saves, occupied landing cells,
turn timers, AI, replication and save/reload.

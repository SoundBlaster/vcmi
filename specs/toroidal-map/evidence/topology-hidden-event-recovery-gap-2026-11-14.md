# Hidden event can prevent in-place Flight recovery

`HeroMovementTest.HiddenEventPreventsInPlaceFlightRecovery` constructs a
synthetic toroidal map with a flying hero above the only remaining land tile.
The test changes the original departure tile to water after movement, then
adds a synthetic visitable `EVENT` below the hero. With the event present,
the in-place landing check, known landing search and both server recovery
paths reject this state; with the event removed, in-place landing succeeds.
The two variants have the same hero position, movement points and terrain.
This demonstrates a remaining difference in recovery behavior and a possible
turn-liveness and information-disclosure concern. It does not demonstrate that
the state arises in ordinary play, or that an unsafe movement step is accepted.

This test records current behavior, not desired acceptance criteria. A fix
must decide how to process an event when a hero lands on it: actual `CGEvent`
visits can start dialogs, battles and further activities. Group recovery and
turn transfer must not continue as if such a visit were already finished, and
landing must not silently discard legitimate event effects. The case remains
open until those interactions and multiplayer replication are addressed.

On macOS arm64, the reproduction and an existing stranded-water-walker test
passed from `f95e0f2` plus the test change. This is a synthetic server fixture,
not a GUI, live multiplayer or real-map observation.

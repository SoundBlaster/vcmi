# Server save, fog and replica movement across a corner

`HeroMovementTest.ServerDiskReloadPreservesBothTeamsFogAfterCornerCrossing`
starts a synthetic two-level toroidal game with two players on different
teams. After the first player moves from `(0,0,0)` to `(35,35,0)`, the test
writes a real server save and loads it with `CGameHandler::load`. It compares
the hero's position, remaining movement points and confirmed layer, plus
every fog cell for both teams on both levels. It also checks a pre-existing
blue-team underground sight cell and that red's surface movement does not
reveal that level or blue's view.

Two replicas are initialized from the loaded state. The loaded handler then
moves the hero back across the corner; its `TryMoveHero` packet is serialized
and decoded separately for both replicas. The test compares the resulting
position, movement points and all fog cells against the authoritative state.
The test server explicitly rebinds its packet application to the newly loaded
state; it does not mutate the pre-load state when continuing play.

On macOS arm64, the test, `CornerMovementRevealsOnlyTheMovingTeamAndLevel`,
and the separate `TcpLobbyStartsToroidalGameForTwoPeers` test passed three
consecutive repetitions on a TB5 build from `258eb0b` plus this test change.
The new replicas and server use the real packet codec in one process, not a
TCP connection. The TCP lobby test is separate: these results do not prove a
networked save/reload, graphical-client resume, remote-host play or packet
confidentiality. No production code was changed in this verification step.

The complete local `vcmitest` suite then passed 1,530 tests without skips,
including the optional older VMAP, save and Complete map fixtures; four tests
remain disabled. The TB5 log is `runtime/full-suite-server-save-fog-2026-11-14.log`.
SpecPM validation reported no errors or warnings.

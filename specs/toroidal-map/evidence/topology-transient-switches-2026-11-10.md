# Switching between Flight and Water Walk

The server allowed an on-foot hero with both movement abilities to switch
between AIR and WATER on a step, but the regular pathfinder did not model
either direction. This could hide a legal path to shore and previously forced
the known-landing safeguard to skip heroes with both abilities.

The pathfinder now considers on-foot AIR → WATER and WATER → AIR transitions
when their destination layers are available. The existing lightweight flying
option still limits taking off to the initial node. This does not add a
transition for boats, and existing layer, terrain and guard checks remain in
effect. The server's conditional known-landing check now also applies when
both on-foot abilities are present.

`HeroCanSwitchFromWaterWalkToFlightToReachShore` compares a planned and
executed WATER → AIR step across a seam to bypass an obstacle and land. The
reverse `HeroCanSwitchFromFlightToWaterWalkAcrossSeam` checks the opposite
transition and executes a landing. Both tests also disable the relevant
pathfinder option and confirm that the corresponding route or layer is not
available. `PathfinderSwitchesTransientLayersAcrossEdgesAndCorners` checks
direct pathfinding transitions in both directions across each axis and all
four corner directions; it places the hero directly and does not execute
those sixteen paths on the server. `CombinedFlightAndWaterWalkPreserveKnownLanding`
checks the server's rejection of one unsafe combined-ability step.

The extended `ToroidalDemoVmapTest.TcpLobbyStartsToroidalGameForTwoPeers`
starts a TCP loopback session, checks that both replicas plan the same
AIR → WATER step, executes it through the server, verifies the replicated
WATER layer, lands, and checks expiry of both one-day abilities on the next
day. The extended test passed three consecutive runs. Remote-network play and
graphical presentation are not established by these results.

On macOS arm64, `vcmitest`, `vcmiserver` and `vcmiclient` built successfully.
The full suite passed 1,487 tests from 226 suites without skips; four remain
disabled. The optional prototype save, VMAP and Complete legacy map were
supplied. The log is `full-suite-transient-layer-switches-2026-11-10.log` in
the TB5 runtime directory. Recovery for already-stranded heroes, turn-timer
behavior and other causes of world changes remain open.

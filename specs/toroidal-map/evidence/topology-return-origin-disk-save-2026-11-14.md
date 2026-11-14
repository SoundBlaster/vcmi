# Recorded transit origin in an on-disk save

`ToroidalDemoGameTest.ServerSaveUsesTheTopologySpecificSerializationVersion`
now additionally exercises the server's recorded transit origin through the
real save-file writer and reader. A hero enters AIR from LAND on a toroidal
map and the server writes a `.vsgm1` file. The test independently reopens the
saved game state and server state, artificially reduces the restored hero's
movement points to one and reveals the map, then requests emergency recovery.
The hero returns to the recorded LAND origin with zero movement points.

The original test still checks the loaded hero's AIR layer, a normal landing,
legacy server-state migration and topology-specific save versions. The
additional disk round trip passed locally on macOS arm64 in a temporary test
filesystem on TB5. It is a synthetic single-player server fixture: it does
not establish actual timer scheduling, graphical save/load, multiplayer
restoration or recovery from older saves that have no recorded origin.

The previous full suite at `e46d07a` passed 1,500 tests with four disabled;
this change adds assertions to an existing test, which passed separately. No
application implementation changed in this step.

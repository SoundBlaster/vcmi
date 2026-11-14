# Timer emergency fallback when the origin is lost

`HeroMovementTest.ExpiredTimerRelocatesStrandedHeroWhenOriginIsLost` starts a
timer before a hero enters WATER. It then simulates an interrupted route and a
world change that makes the recorded departure tile water, leaving one movement
point and another known safe land tile. The test directly advances the timer:
the hero reaches that alternative tile on LAND with zero movement points, and
the selected hidden tile remains hidden.

On macOS arm64 the test passed together with the existing timer/no-landing,
ordinary timer landing and old-save fallback tests (four tests in total). The
`vcmitest` target was built on TB5 from `dfcde31` plus this test. An initial
run from the source directory failed in global setup because VCMI could not
find `CONFIG/FILESYSTEM`; the successful run used the build's `bin` directory,
which supplies the expected `config` and `Mods` links.

This is a synthetic state and directly driven timer, not proof of wall-clock
scheduling, GUI presentation or turn transfer. Recovery when no suitable known
land exists remains unresolved.

`HeroMovementTest.ExpiredTimerRetriesEmergencyRelocationAfterLandIsRevealed`
constructs an on-foot WATER state with one hidden safe land tile and no other
land. Two direct timer updates leave the hero in place while the tile is
hidden. The test then directly changes that tile's visibility: an update of
one millisecond leaves the hero in place, and a subsequent 999-millisecond
update recovers the hero to LAND with zero movement points. Together with the
previous timer fallback and no-safe-landing tests, three focused tests passed
on macOS arm64 from `045eb3c` plus this test. A first fixture started the hero
on water and failed even with a direct recovery call; the passing fixture
starts the hero on land, asserts no boat, and changes the terrain afterward.
This does not establish the precise cause of the initial failure, actual
fog-reveal packets, timer wall-clock behavior or all vehicle cases.

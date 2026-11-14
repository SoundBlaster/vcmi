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

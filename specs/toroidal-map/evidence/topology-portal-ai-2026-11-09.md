# One-way portal AI neighborhood

On 2026-11-09, three Nullkiller2 portal behaviors were changed to enumerate
adjacent cells through the map topology: vacating a portal exit, selecting a
tile guarded near an entrance, and deciding whether a guard traps a hero on an
exit. Bounded maps still discard outside coordinates. These changes are not a
claim that the AI profile supports every toroidal mechanic.

- Source baseline `523a378`, plus the changes identified below.
- The checkout-specific TB5 Release build of `vcmitest`, `vcmiclient`,
  `vcmiserver`, and `vcmieditor` completed with exit 0 on macOS arm64.
  Log: `portal-ai-build.log` in the TB5 build root. Existing duplicate-library
  and absent search-path linker warnings appeared.
- `vcmitest --gtest_filter=Nullkiller2_Behaviors_OneWayPortal.seamExitIsNotTrappedWhenWrappedNeighborIsSafe`
  exited 1 during global initialization because game resource `DATA/LCDESC`
  was absent. The test body did not run; neither this scenario nor bounded
  portal behavior is verified by this build.

```text
84b2209646d7470734e1a9798ddd1d471e08dd7cc78ca6ad8f1812f8394dad03  AI/Nullkiller2/Behaviors/ExplorationBehavior.cpp
487ecd5bf397ea71a22e92601317c140e9b1c50e705e4f44dfc8f1edf8414125  AI/Nullkiller2/Goals/AttackOneWayPortalGuard.cpp
313f6a4a150de322ab5c2ba06ca59af0e65abe4d87cb73e682f11bceba75210e  test/nullkiller2/Behaviors/OneWayPortalBehaviorTest.cpp
```

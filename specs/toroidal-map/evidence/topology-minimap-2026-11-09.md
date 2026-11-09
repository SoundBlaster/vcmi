# Minimap wrapped intervals

On 2026-11-09 the minimap's split interval calculation was moved into a shared
testable helper. The Cartesian product of the returned horizontal and vertical
parts is used for the radar rectangle. Intervals at least as wide as a map
period collapse to one full-axis part.

- Source baseline `720d0af`, plus the changes identified below.
- The checkout-specific TB5 Release build of `vcmitest`, `vcmiclient`,
  `vcmiserver`, `vcmieditor` and `vcmi-map-topology-test` completed with
  exit 0 on macOS arm64. Log: `minimap-interval-build.log` in the TB5 build
  root; existing linker warnings appeared.
- The asset-independent topology executable passed **7 GoogleTest cases**.
  The new interval case checks 5,620 combinations of extents, starts and
  lengths against independently enumerated positions, verifies that each
  result is in bounds and is visited once, and checks negative length.
- The minimap has not been rendered or interacted with; pixel scaling,
  borders, input and visual continuity remain unverified.

```text
bed650f6bf3eb7b0573d5b83729bae838af74e4820d432459df2e4e2d4168341  lib/mapping/MapTopology.h
66dd132d78b3cfe7dac4fbcbd28537b485634a0261b780aeedef2e2651dff764  client/adventureMap/CMinimap.cpp
82cf2f486d16ccd51770b691604e73dfbeb25a585172249b97959eea3a49da1e  test/map/MapTopologyTest.cpp
4aeed6490f449419a910838b8960c608ff2fe6a76d947844972e752b665012ef  cmake/test/vcmi-map-topology-test (TB5 build root)
```

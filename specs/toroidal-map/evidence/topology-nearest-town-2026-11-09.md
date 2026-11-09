# Nearest town across a seam

The shared nearest-town helper now uses toroidal horizontal displacement when
the map wraps. It retains the existing two-dimensional comparison between
towns on different map levels, as well as existing tie handling. Both the
Town Portal and Reinforcements effects use this helper. Only the Town Portal
scenario below has been exercised end to end.

On 2026-11-09, macOS arm64, a new unskilled Town Portal test first failed:
the hero at x=0 travelled to the interior town at x=7 rather than the town
at x=33 across the seam (`town-portal-before-tests.log`). After the fix,
the test checked that the bounded map selects x=7 and the toroidal map selects
x=33. The Release `vcmitest` build succeeded (`town-portal-after-build.log`),
and `TownPortalTest.*:HeroMovementTest.*:MapTopology.*` passed 28 tests with
copied Complete data (`town-portal-after-tests.log`). Reinforcements,
cross-level town selection and GUI casting were not exercised in this run.

Source baseline `cd847d3` with local changes:

```text
b882cfc4ecf0e11a9a3e5f51e5361eda68a489cbaa409c2c94bb47b9883d69ca  lib/spells/adventure/TownRelatedAdventureSpellEffect.cpp
0ffe6627cdbba569491ce2f44dd4afefc51701abf647b7b9eb3be93e26ce962a  lib/spells/adventure/TownRelatedAdventureSpellEffect.h
5f0792933e8cb36da5eec165ce2172123401b464194bf9d0f4a2706523a0f2a7  lib/spells/adventure/TownPortalEffect.cpp
d45f8eb7e56e3a3cd79d8f9933d2b54684e000bad0578b19b97493ee11851631  lib/spells/adventure/ReinforcementsEffect.cpp
5012391e8741d02f978668f60e1e925560475bea9bbe01fc8446d9ef4499c668  test/server/activities/ActivityProcessorTest.cpp
```

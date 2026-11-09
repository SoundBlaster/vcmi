# Shared pixel wrapping: build and isolated unit run

On 2026-11-09, the adventure camera, minimap and moving-object rendering
were changed to use the same positive-modulo helper as map coordinates. This
avoids separate implementations of negative and multi-period wrapping.

- Source baseline: commit `e4108d93e9f839e0d16b4d4aa2062b8fb2ede29e`, plus
  the source changes identified below.
- The checkout-specific TB5 CMake Release build completed with exit 0 for
  `vcmitest`, `vcmiclient`, `vcmiserver` and `vcmieditor`. The preceding
  compilation log is `pixels-build.log` in that build root; a subsequent
  incremental invocation completed with exit 0. The linker emitted duplicate
  library and absent Conan search-path warnings.
- The independent GoogleTest executable described in
  `topology-unit-2026-11-09.md` was rebuilt against these sources and passed
  **4 tests**, including pixel positions beyond either edge by more than one
  period. This checks arithmetic, not rendered scrolling or gameplay.

```text
cc0152a6b327ee123554f073dd25d5407c27b14d549a3c3dacd2c4a89044c302  lib/mapping/MapTopology.h
0a9b56f58036da9d3ceec11dd72fbc05ea66549bac154566d5493728b02a7dee  test/map/MapTopologyTest.cpp
883591ee2674277f597be31b0ec5a68f245a103ffd8478e8659d83c7eeafb441  client/mapView/MapRendererContext.cpp
d0df7ecdcd40c2944ad21397723c7719128fb0ae71734fd3e43128bc38a4df13  client/mapView/MapViewController.cpp
5ab271d30228510ffef05be2b332e06704532d6c0e3b397d4314d9ce79f43cfe  client/adventureMap/CMinimap.cpp
0aefea4ce21cfe9700c442adc977cbe2b451b97d0b3e4d29c44c378d692a3a84  tools/map-topology-tests (TB5 build root)
```

Game assets were unavailable, so integrated tests and visual verification
remain outstanding.

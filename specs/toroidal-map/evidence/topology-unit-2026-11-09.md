# Map topology helper: isolated unit run

On 2026-11-09, compiled `test/map/MapTopologyTest.cpp` with the repository's
GoogleTest source and `gtest_main.cc` as an independent executable. It uses the
actual VCMI `MapTopology` and `int3`, but does not initialize VCMI game state.
This run therefore checks coordinate arithmetic only, not movement, routing,
rendering, persistence or any full-game scenario.

- Source baseline: commit `a41d64911826ae0f33649e1ff5480d82b3025dc9`, plus
  the then-uncommitted source files identified below.
- Host: macOS arm64, Apple clang 21.0.0 (`clang-2100.3.34.2`), C++20.
- GoogleTest: repository submodule at
  `b514bdc898e2951020cbdca1304b75f5950d1f59`.
- Build output and dependencies: checkout-specific TB5 build root
  `/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973`.
- Compiler exit 0; GoogleTest headers emitted `char8_t` to `char32_t`
  conversion warnings. Executable exit 0: **3 tests passed**.
- Checks: wrapped edges and corners at two levels, invalid levels, bounded
  rejection, and short displacement across seams. Tiny-map policy and actual
  game behavior remain unverified.

```text
94d5978610b5c0e8735ecd5e44631cbbfe066ae835a880218b3e77fa5e846fbb  lib/mapping/MapTopology.h
70dd165c13206945877ca7cdd40476b9382c8c9a9d5a717d5dc4e0a78f8fe301  test/map/MapTopologyTest.cpp
1d290cd24bd2bda08fdc5e3396f19066ee97e4ae75d72bd65ca6166009e7fc1f  tools/map-topology-tests (within the TB5 build root)
```

The compile used `/usr/bin/clang++ -std=c++20`, the repository include paths,
the pinned submodule's `gtest-all.cc` and `gtest_main.cc`, and the Boost, TBB
and LuaJIT headers restored into the TB5 Conan cache. Running the resulting
executable reported `3 tests from MapTopology` and `[  PASSED  ] 3 tests.`

The full CMake Release build of targets `vcmitest`, `vcmiclient`, `vcmiserver`
and `vcmieditor` also completed with exit 0 on the same source snapshot. Build
directory: `cmake` under the TB5 build root above; `cmake --build <directory>
--target vcmitest vcmiclient vcmiserver vcmieditor -j 4`. The log is
`build.log` under that build root. The linker warned about duplicate `-lbz2`,
`-lz`, `libgtest.a` and two absent Conan system-library search paths. This build
result is not a game launch or evidence of integrated toroidal behavior.

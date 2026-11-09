# Standalone topology CTest

On 2026-11-09 a dedicated CMake target was added so the coordinate tests can
run through CTest without VCMI's asset-dependent global test environment.
This is a narrow arithmetic check, not a gameplay or visual test.

- Source baseline `f99e209`, plus the CMake change identified below.
- macOS arm64, Apple clang 21.0.0, CMake 4.4.4, Release build, checkout-specific
  TB5 build root `/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973`.
- `cmake --build <build-root>/cmake --target vcmi-map-topology-test -j 4`:
  exit 0.
- `ctest --test-dir <build-root>/cmake -R '^MapTopologyStandalone$'
  --output-on-failure`: exit 0; one CTest executable passed, containing four
  GoogleTest cases.

```text
25476db6537790c019b5251bcde54c005c1539d16afa48594245dfecec4cdf70  test/CMakeLists.txt
0a9b56f58036da9d3ceec11dd72fbc05ea66549bac154566d5493728b02a7dee  test/map/MapTopologyTest.cpp
3b9d5843ece26bf8cb188039a82aee986947e28b1eaf6100ed57e6d205b543ab  cmake/test/vcmi-map-topology-test (TB5 build root)
```

Full `vcmitest` still requires the original game resources and remains unrun.

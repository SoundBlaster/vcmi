# Toroidal range enumeration

On 2026-11-09 the toroidal branch of `getTilesInRange` was changed to use a
shared range enumerator. It visits each canonical tile once and retains the
level; the existing fog visibility filter remains in the callback. This does
not prove the integrated fog-of-war behavior or its information boundaries.

- Source baseline `7c10048`, plus the changes identified below.
- On macOS arm64 with Apple clang 21.0.0, the checkout-specific TB5 Release
  build completed with exit 0 for `vcmitest`, `vcmiclient`, `vcmiserver`,
  and `vcmieditor`. Log: `sight-range-build.log` in the TB5 build root;
  existing duplicate-library and absent search-path linker warnings appeared.
- The asset-independent `vcmi-map-topology-test` executable passed **6
  GoogleTest cases**. Its range comparison exhaustively visited all cells on
  maps of width and height 3 through 8, for radii 0 through 10 and each of
  four distance formulas: 1,584 parameter combinations, with a repeated-image
  center on level 1. The test independently computes the shortest x/y
  distances, checks returned sets and rejects duplicate visits. A separate
  case checks invalid levels and negative radius.
- `ctest --test-dir <build-root>/cmake -R '^MapTopologyStandalone$'
  --output-on-failure` exited 0 for the same test executable. The full
  asset-dependent game tests remain unexecuted.

```text
802e3b9878ec381654e6d9542176598cce7176fa6d247d85bcc8b56bf6925f6b  lib/callback/CGameInfoCallback.cpp
6bc39d140be3695032bcc7290f559c82c117472e7fa1b1fa4f55dea2a3862e81  lib/mapping/MapTopology.h
9a5333f38d9928b9018282fd4471c2d097af569ebab3bdd0e57808d651e85467  test/map/MapTopologyTest.cpp
ece507d7de0435fef77e14c5f2ccc3d3761425e383957b3947fb9e0c18a548ba  cmake/test/vcmi-map-topology-test (TB5 build root)
```

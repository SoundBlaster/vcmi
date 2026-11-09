# Static seam-object pilot restriction

On 2026-11-09, map loading, saving and editor validation were changed to
identify static objects whose visible, blocking or visitable mask cells fall
outside the map rectangle when toroidal topology is selected. Empty mask cells
are allowed outside. Heroes and boats are excluded from this check because they
use a separate moving-object path; that exclusion is not proof of their
correctness. This is a provisional authoring restriction, not support for
static objects occupying both sides of a seam.

- Source baseline `97e31e8`, plus the changes identified below.
- The checkout-specific TB5 Release build of `vcmitest`, `vcmiclient`,
  `vcmiserver`, `vcmieditor`, and `vcmi-map-topology-test` completed with
  exit 0 on macOS arm64. The log is `seam-validation-build.log` in the TB5
  build root. Existing duplicate-library and absent search-path linker warnings
  appeared.
- `vcmitest --gtest_filter=MapObjectDrawOrderTest.RejectsStaticOccupiedFootprintsAcrossToroidalSeams`
  exited 1 before the test body during global initialization because game
  resource `DATA/LCDESC` was absent. Thus the new object tests, the editor
  messages and the load/save rejection are compiled but not verified at
  runtime.
- `ctest --test-dir <build-root>/cmake -R '^MapTopologyStandalone$'
  --output-on-failure` exited 0: one CTest executable passed. It checks
  coordinate arithmetic only, not the new object validation.

```text
7436c416add16cb1c794f9e90ddf913df7bca50a5a3850395ab2e1275e9ae613  lib/mapping/CMap.cpp
53874084024b8292a7362fffaf4b9cba68d320f6a8145c9d7459307020a98e2d  lib/mapping/CMap.h
05067180ef2de8099fd682f45d1ea1a49cb644c0278b8a618393895b33b276e4  lib/mapping/MapFormatJson.cpp
995944b2a8e0699384fddc4f0e003a1ec3a4368c27b59c21ed648cfd38644b8d  mapeditor/validator.cpp
01d7c4d39afbc49eb9f58bdb64ad0ace7abdd37a4ae6bec21ff708dd98386fe7  mapeditor/translation/russian.ts
9bd012d5892ce507251f7feea4329b2006192ab131b427cd6046f6062477cbb2  test/map/MapObjectDrawOrderTest.cpp
```

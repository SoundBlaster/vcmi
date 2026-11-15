# Camera level bound

`MapViewController::setViewCenter` used the map's number of levels as the
inclusive maximum for `std::clamp`. `MapInfoCallback::getMapSize().z` is a
count, so the former expression could retain level 1 on a one-level map or
level 2 on a two-level map if called with such an input. The ordinary level
switching UI already cycles through valid levels; a user-reachable source of
the invalid input has not been demonstrated.

The controller now clamps to `levels - 1` on both bounded and toroidal maps.
The client and test binaries built on macOS arm64 from `d62456b` plus this
one-line correction. Twelve existing map-view/topology tests and three existing
game-state tests for underground movement, team visibility and minimap hero
visibility passed. Reports are `runtime/view-level-topology-2026-11-15.xml`
and `runtime/view-level-game-2026-11-15.xml` under the TB5 build root. The
build emitted existing duplicate-library and missing Conan search-path
warnings.

These tests exercise valid map levels and geometry; they do not call the
controller with the invalid level or verify interactive level switching on
the new build. The bounds error is established by source inspection, not a
reproduced crash. A real map is expected to have at least one level.

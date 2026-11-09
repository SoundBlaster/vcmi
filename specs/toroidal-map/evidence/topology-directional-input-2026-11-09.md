# Directional hero input

Directional hero shortcuts previously rejected a destination outside the map
rectangle before asking the pathfinder. They now canonicalize the requested
neighbor through the map topology: bounded maps still reject outside positions,
and toroidal maps pass the wrapped coordinate to the existing pathfinder.
Movement authority and path costs remain with the existing game logic.

On 2026-11-09, macOS arm64, the Release `vcmiclient` build completed on TB5
(`directional-shortcut-build.log`). The existing `MapTopology.*` and
`HeroMovementTest.*` suites passed 24 tests with copied Complete game data
(`directional-shortcut-tests.log`). They exercise the underlying coordinate
and movement logic, not the keyboard handler itself. No directional key was
observed moving a hero across a seam in the running client; this remains a
required user-facing test. The build emitted the existing duplicate-library and
missing Conan search-path warnings.

Source baseline `fcc19bb` with the following local change:

```text
b54516529ffbb6b35da44beb5e8fb232b023297eddf13aa5065094d56469f827  client/adventureMap/AdventureMapShortcuts.cpp
```

# Toroidal map editor setting: build record

On 2026-11-09 a translatable checkbox was added to the map editor's General
settings. It selects the toroidal flag on each level, disables the control for
maps narrower or shorter than three tiles, and recalculates guard positions
when the flag changes. It does not generate matching terrain or roads and does
not validate the complete seam.

Source baseline: `0ceae91` plus the then-uncommitted files identified below.
The `vcmieditor` Release target built successfully with exit 0 on macOS arm64;
the log is `editor-build.log` under the TB5 build root specified in
`topology-unit-2026-11-09.md`. The Qt translation compilation reported 958
Russian translations generated; the linker repeated its system-library search
path warnings. No GUI, save/reopen or game test was performed because the
original game data is unavailable.

```text
81d41ed97724de39e1f49eb010b9ab84bc4589ca958fa21ff7e9067efc26e58a  mapeditor/mapsettings/generalsettings.cpp
44e20a28770d01114d6c803aceccadcd79f899daca5545589c85b26b40f91a93  mapeditor/mapsettings/generalsettings.ui
add0bc14fe7b483fd642a71ac0ba4e3fd2013b0d99a94b6243e619d3196e9864  mapeditor/translation/russian.ts
```

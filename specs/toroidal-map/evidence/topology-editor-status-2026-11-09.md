# Editor feature status

On 2026-11-09 the editor's topology control was labelled experimental, and
its tooltip was updated to mention the static-object restriction and that AI
and multiplayer have not been verified. A Russian translation was supplied.
This is an authoring hint, not a warning at AI selection or a restriction on
starting a game.

- Source baseline `f172cfc`, plus the changes identified below.
- On macOS arm64, `cmake --build <TB5-build-root>/cmake --target vcmieditor
  -j 4` exited 0. Log: `editor-experimental-build.log` in the build root.
- The editor was not launched with game data; the translated layout and
  tooltip were not visually verified.

```text
0bfe4e47cc37106bb88004488f8b5f9b765c528cf4326e918de0abf23b9d6c0f  mapeditor/mapsettings/generalsettings.ui
1b9ddb62dcf7833cae2eabffbef4c7ccfebd47c40984aa857728e91116d9ee96  mapeditor/translation/russian.ts
```

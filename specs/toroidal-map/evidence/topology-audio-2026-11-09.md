# Ambient sound distance

On 2026-11-09, ambient sound attenuation was changed to use the short
toroidal displacement for visible sound sources across a seam. Bounded maps
retain their original distance calculation. This is a partial audit of audio,
not proof of audible continuity.

- Source baseline `f298937`, plus the source change identified below.
- On macOS arm64, `cmake --build <TB5-build-root>/cmake --target vcmiclient
  -j 4` completed with exit 0. Log: `ambient-build-fixed.log`. An initial
  build failed because the topology header was missing; the include was added
  and the build rerun. Existing linker warnings remain.
- The shortest-displacement arithmetic has isolated tests as recorded in
  `topology-range-2026-11-09.md`; the audio player itself has not been
  executed or listened to because original game data is unavailable.

```text
dd695276da49f2a904faa1be29e6854f7045868eba2ea219914066d1f071daf8  client/adventureMap/MapAudioPlayer.cpp
```

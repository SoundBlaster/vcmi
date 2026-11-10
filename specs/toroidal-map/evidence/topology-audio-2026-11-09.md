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

## Object sound sources across a seam

On 2026-11-10, `MapAudioPlayer::addObject` still clipped sound sources to the
rectangular map. This discarded the wrapped portions of terrain overlays and
static blocking objects. The player now indexes source positions through
the map topology, deduplicating repeated canonical tiles. Visitable objects
continue to use their visitable tile; bounded maps still discard off-map
positions.

`HeroMovementTest.AmbientSoundSourcesFollowWrappedObjectFootprints` checks an
overlay, blocker and visitable object at a corner of the underground level,
including bounded comparisons. That test and two related object and terrain
tests passed. The macOS arm64 client and test targets built; the linker
reported existing duplicate-library and missing Conan search-path warnings.
The extracted source-tile calculation was exercised, not the sound mixer or
actual audible playback. Original assets were found after the earlier note,
but audible continuity is still unverified.

# Original VCMI reader and experimental formats

An instrumented test executable linked against the original VCMI engine at
`30e9207276f137658da5132705c05644bfe33aee` exercised the actual legacy
`CMapService` and `CLoadFile` implementations on macOS arm64. The added
[test-only probe](../tools/legacy-format-probe.inc) read existing files on TB5:

- The tagged toroidal map `toroidal-water-vertical-crossing-lab.vmap`, SHA-256
  `78e95df86ca57a7c799884b4628b0c8246f25f3af7e550ce5d17e1d7743aad5e`,
  was rejected with `Unsupported map format version` when loading its header.
- The tagged `Toroid Open Water Corner` save `Autosave-111.vsgm1`, SHA-256
  `0846265548445959b6c1604e0edede70626dd2535f7dd65e9340ce7b95231653`,
  was rejected by `CLoadFile` with `SaveTooNewException`.
- The original engine's existing `SaveGameTest.savedGameIsRestored` passed,
  as did loading a version-3 bounded map header. Both `test_terrain.vmap` and
  the corrected single-island copy described below were used as controls.

The baseline production engine code was unchanged; its test harness was
temporarily extended and restored after the run. A missing Google Test source
directory was temporarily linked to the existing checkout, then restored.
The test precompiled header and two test objects were rebuilt and the test
executable relinked with the existing baseline engine objects and libraries.
The linker reported duplicate libraries and missing search directories.
These are library-level checks, not an old GUI loading or playing the files,
a network compatibility test, or a guarantee about future upstream versions.

The previously prepared `baseline-single-island-flight-waterwalk.vmap` had no
topology field but had the experimental toroidal `versionMajor` marker, so
the original reader rejected its header. This invalidates the description of
that particular file as a ready-to-load bounded GUI fixture. Its SHA-256 is
`9998340afe54fccdb6ae243b0765fe3df709c3056f02f27912acf7f2c16e84bd`.
A separate copy, `baseline-single-island-bounded-compatible.vmap`, changes
only `versionMajor` to 3; SHA-256
`1ca0b8e5b27dee3d07d9962628663aabdb5c4bbdb736152d5129700c4fa8ff57`.
The original reader accepts that copy's header. Its complete map loading and
gameplay remain unverified. The earlier in-memory single-island experiments
used generated maps, not this GUI fixture, and are unaffected by this finding.

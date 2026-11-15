# Automated suite and bounded-save repeat

On macOS arm64, `vcmitest` was rebuilt from clean commit `40bffe9` using
the checkout's TB5 CMake build. The default test preset ran 2,098 tests from
250 suites: 2,084 passed and 14 were skipped; three additional tests were
disabled. There were no failures. The skipped cases require explicitly supplied
external saves, maps, servers or output paths. The log and XML are
`runtime/full-suite-40bffe9-2026-11-16.{log,xml}` under the TB5 build root.
This is the configured automated suite, not an exhaustive gameplay or GUI
regression. The linker again warned about duplicate libraries and missing
Conan library search paths.

The two tests for an original VCMI bounded autosave were then run separately
against the recorded local file SHA-256
`879f68ef85f95a08a9b17238daed0d73058fcf65dc2e7c42bb6cd8564c6b6d4d`.
With the original `core`/`vcmi` mod profile selected via
`VCMI_TEST_MODDED_ROOT`, both passed: the map remained bounded, and a hero
moved after decoding and after normal server loading. The source save's hash
was unchanged. XML:
`runtime/original-bounded-40bffe9-correct-profile-2026-11-16.xml`.

An initial repeat using the default test preset passed the direct decode but
the normal loader rejected the active mod set with `ModIncompatibility`.
The default preset includes `vcmi-test`, unlike this save's profile; the
successful repeat selected the matching profile rather than bypassing the
normal mod-compatibility check. Neither test is a graphical reload or a full
continued game. See the [original bounded session](topology-old-network-peer-2026-11-15.md#reading-the-original-servers-bounded-autosave)
for how the save was created and other limits.

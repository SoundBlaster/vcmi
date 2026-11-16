# Automated suite after teleport resolution

On macOS arm64, the TB5 `vcmitest` binary built from `c74e968` with the
subsequently committed Earth mine-access test changes ran the configured
default suite in an isolated data profile. Of 2,105 tests in 252 suites,
2,090 passed and 15 were skipped; three further tests were disabled. There
were no failures. This is the automated suite, not an exhaustive playthrough
or a visual assessment. The binary's build-time version stamp predates the
mine-access commit `40b6b09`; the test source used was the same as that
commit. No runtime code changed between `c74e968` and `40b6b09`.

The skipped cases require optional external saves, maps, output locations or
live servers. They include the original bounded save and the old/current
server probes; the suite did not exercise those external fixtures in this
run. The [previous-framed server lobby check](topology-acknowledgement-review-2026-11-14.md#resuming-a-deferred-teleport-route)
and [Earth mine capture](earth-object-overlap-audit-2026-11-15.md#mine-approach-repaired)
were executed separately, with the limitations stated there.

After the suite, the two original bounded-save tests were repeated separately
with the source fixture SHA-256
`879f68ef85f95a08a9b17238daed0d73058fcf65dc2e7c42bb6cd8564c6b6d4d`
and an isolated `core`/`vcmi` mod profile. Both passed: direct deserialization
and normal `CGameHandler::load` retained a bounded 36×36 map, and an in-process
server handler moved the blue hero from `(19,18,0)` to `(20,18,0)` with fewer
movement points. The source save hash was unchanged. The XML and log are in
`runtime/original-bounded-current-2026-11-16/` on TB5. This does not exercise
a graphical reload, a network lobby or a complete resumed turn.

The log and XML are in `runtime/full-suite-teleport-resolution-2026-11-16/`
under the TB5 build root. The build emitted the existing duplicate-library
and missing Conan search-path linker warnings.

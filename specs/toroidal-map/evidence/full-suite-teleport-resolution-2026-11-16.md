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
were executed separately, with the limitations stated there. The earlier
[original bounded save check](full-suite-40bffe9-2026-11-16.md) predates the
teleport-resolution change and was not repeated in this run.

The log and XML are in `runtime/full-suite-teleport-resolution-2026-11-16/`
under the TB5 build root. The build emitted the existing duplicate-library
and missing Conan search-path linker warnings.

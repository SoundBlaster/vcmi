# Nullkiller2 end-turn retry backoff

When an end-turn request is rejected, Nullkiller2 retains the turn and retries.
Previously it could send requests in a tight loop, including when a transient
hero cannot safely settle. It now waits approximately one second between
attempts, releases its game-state shared lock while waiting, and checks for
interruption at 100 ms intervals. This applies to bounded and toroidal games.
It limits repeated requests; it does not guarantee recovery or turn completion.

On macOS arm64, `Nullkiller2` and `vcmitest` built from `4fc4fea` plus this
change. Fifteen focused tests passed. In the new scripted-client test, the
first end-turn request is rejected and the second accepted: the test checks
that an exclusive game-state lock can be acquired during the delay and that
the two requests are at least 800 ms apart. Two additional tests exercise
the wait timeout and notification when the turn ends. This fixture does not
establish behavior with a live network or guarantee a hard real-time bound.

The full local suite passed 1,524 tests; three tests requiring optional
external fixtures were skipped and four remain disabled. The TB5 log is
`runtime/full-suite-ai-endturn-backoff-2026-11-14.log`. SpecPM validation
reported no errors or warnings.

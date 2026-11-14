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

The three skipped optional compatibility tests were then run separately on
the same build with the older untagged laboratory save, untagged laboratory
VMAP, and the Complete `Peacemaker.h3m`. All three passed. Their input
SHA-256 values were unchanged across the run:

```text
ad2a9df20f1995f62ffd215d62a6e8fa8cb4856443aa9302144f19d864d3ca73  laboratory save
3fbc0a2cbbdfa22068e7e6a099a7b51e803d39b5fc0d0b3fd95155cc6eeb232d  laboratory VMAP
61bd6b10168f00f0ac27a39b92874f934595d9f292633cc73fa8ab622ed9ea48  Peacemaker.h3m
```

The save test directly decodes its contents; it does not demonstrate a GUI
resume or bypass the separate mod-compatibility check. The map tests perform
in-memory format conversions, not full gameplay on those maps.

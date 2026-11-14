# Diagnostic command channel verification

The macOS arm64 Release client and `vcmi-map-topology-test` were built from
the implementation on TB5. The topology executable passed all 26 tests,
including two tests of the real local Unix socket transport. Those cover
permissions, valid and invalid requests, oversized and multiple requests,
the incomplete-request timeout and subsequent recovery, and preservation of
an existing file. The callback in these tests is a test double; they do not
prove delivery to game widgets or successful in-game actions. SpecPM package
validation passed without errors or warnings.

An earlier build of the command-enabled client was packaged as a separate
development-signed macOS app while retaining the existing game session. It
initially waited in `open` during resource filesystem initialization. After
the user granted the requested macOS access, it opened its window and socket.
This sequence does not establish the cause of the prior wait.

On that running client, socket click commands opened New Game and Scenario,
sorted by date, and started the 36×36 toroidal route-safety map. A `cast`
shortcut opened the spellbook and a click cast Water Walk; observation
records show selected-hero mana changing from 100 to 92. A `west` shortcut
moved the hero from `(15,16,0)` to `(14,16,0)` and reduced movement points
from 1,560 to 1,460. A preceding `south` shortcut at the shore was accepted
for dispatch but did not move the hero; that alone does not identify which
gameplay rule rejected the move. The trace is
`runtime/observation-command-test.jsonl` under the TB5 build tree.

The running package was built before the final captured-input guard and
close-on-exec hardening. Those later changes were built and tested separately,
but the exact final binary was not exercised through this interactive run.
The remaining route and fog scenarios require their own gameplay evidence.

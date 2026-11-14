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

## Cursor-dependent widgets

The original diagnostic click sent a mouse-move event with the requested
coordinates but left `InputHandler`'s stored cursor position unchanged.
`SelectionTab` reads that stored position when choosing a scenario, so the
dispatched click could select a different row. Diagnostic clicks now update
the cursor through `InputHandler::setCursorPosition` before pressing and
releasing the mouse button, as ordinary mouse input does.

The macOS arm64 client containing this change was rebuilt on TB5, signed, and
launched using the same local test app. In the actual scenario list, successive
socket clicks at `(470, 320)` and `(470, 395)` selected the fifth and eighth
visible rows respectively, and the detail pane displayed their corresponding
scenario names. The topology and command-channel executable passed all 26
tests. These observations validate those interactions; they do not imply that
an `accepted` response guarantees an action succeeded or that every widget has
been exercised.

## Pointer movement without a click

The `hover X Y` command uses the same bounded logical coordinates and stored
cursor-position update as `click`, without pressing or releasing a button.
On a signed macOS arm64 client built from `47fa818` plus this change, hover
over New Game highlighted the menu item without opening it. In an ordinary
bounded-map session, hover over a visible fountain and then a town changed the
status text to the respective object without moving the selected hero. A
negative x coordinate returned `error invalid_command`. The real socket
transport tests are independent of the GUI dispatch and do not verify every
hover receiver. Hovering an unseen tile retained the preceding object's status
text on that build; the [separate legacy UI correction](statusbar-fog-hover-2026-11-14.md)
addresses this.

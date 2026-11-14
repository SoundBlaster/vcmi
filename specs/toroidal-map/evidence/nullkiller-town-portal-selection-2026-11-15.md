# Nullkiller Town Portal selection after a rejected goal

Nullkiller's Town Portal goal previously stored its intended town in the
gateway before checking whether the town was occupied or the cast was locally
valid. If the goal threw before sending a request, that ID remained available
to a later map-object selection dialog. The dialog uses the stored ID when it
appears among the offered objects, so such a stale match could select an object
instead of cancelling. The stale ID was reproduced; an occurrence during a
normal game has not been established.

The goal now publishes the town only after its local checks succeed, just
before submitting the spell request. In a synthetic fixture with a visiting
hero and garrison in the intended town, the rejected goal left the town ID
behind before this change. After the change, on both bounded and toroidal
maps, the rejected goal leaves no selected ID. A subsequent unrelated
selection dialog offering the same town sends a cancellation through a
recording client. This test does not run an actual server or establish
recovery after a request was already submitted.

A second synthetic test covers that separate case. A recording client accepts
the spell request and observes the intended town ID during submission; the
test supplies the negative acknowledgement state that a real client would
report. Previously the failed goal still retained the ID. A scope guard now
clears it when the cast action exits, including on failure, after waiting for
the action's dialogs. The test checks the request was actually sent and the
ID is then cleared. It does not simulate packet transport or exercise an
actual server rejection.

The test was later extended with a scripted ordering in which the recording
client opens an asynchronous object-selection dialog while the spell request
is being sent, then reports the preset rejection state. The gateway's dialog
task sends the intended town ID before the cast action clears its selection.
The five focused Town Portal and dialog tests passed three repeated runs.
This is an ordering probe using a test double, not a claim that the real server
produces that particular sequence.

`Nullkiller2_AIGateway.townPortalDialogSelectsWrappedTownThroughServer` then
exercises the actual server handler with an expert Earth Magic hero and two
towns on a revealed toroidal map. A test client serializes requests into an
in-process `CGameHandler`, forwards the server's selection dialog and
acknowledgements to Nullkiller, and lets its asynchronous dialog task answer.
The server moves the hero from `(0,10)` to the selected town near the opposite
edge, spends mana and movement points, and the gateway ends with no pending
question or selected object. The new test and five related Town Portal/dialog
tests passed ten repeated runs. The client and server share an in-process
game state; request delivery is a test adapter, not TCP, a graphical session,
or a full AI turn.

The gateway's selected object is shared between the goal and asynchronous
dialog task. To avoid unsynchronized reads and writes when the goal cleanup
and dialog overlap, it is now atomic. The dialog takes and clears the value
with one `exchange` before answering, rather than writing it again after the
answer acknowledgement; this also avoids a late clear by that dialog task.
The five focused tests, including the scripted asynchronous selection, passed
ten repeated runs. This is a code-level synchronization correction and a
local functional check; ThreadSanitizer and real network scheduling were not
tested. The complete local suite with this change passed 1,646 tests with
four disabled using the same external fixtures; its log is
`runtime/full-suite-atomic-town-selection-2026-11-15.log` under the TB5 build
root.

On macOS arm64, `vcmitest` and Nullkiller2 rebuilt on TB5. The focused Town
Portal and dialog tests passed three consecutive runs after the dialog check
was added. A complete local run with the same implementation and the earlier
version of the regression test passed 1,645 tests with four disabled; its log
is `runtime/full-suite-town-portal-target-2026-11-15.log` under the TB5 VCMI
build root. The later test-only extension was checked in the focused run.
That first run predates the second cleanup change. The focused Town Portal
and dialog tests for both changes passed three times after it; that run did
not exercise the real server's spell dialog and reply.
With the second change, a complete local run with the same external fixtures
passed 1,646 tests with four disabled. Its log is
`runtime/full-suite-town-selection-cleanup-2026-11-15.log` under the same
build root.
This is local evidence, not hosted CI or a complete audit of asynchronous
selection and cast acknowledgements.

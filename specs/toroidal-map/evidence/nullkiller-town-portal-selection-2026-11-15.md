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

On macOS arm64, `vcmitest` and Nullkiller2 rebuilt on TB5. The focused Town
Portal and dialog tests passed three consecutive runs after the dialog check
was added. A complete local run with the same implementation and the earlier
version of the regression test passed 1,645 tests with four disabled; its log
is `runtime/full-suite-town-portal-target-2026-11-15.log` under the TB5 VCMI
build root. The later test-only extension was checked in the focused run.
That first run predates the second cleanup change. The focused Town Portal
and dialog tests for both changes passed three times after it; they do not
establish the ordering of a real asynchronous spell dialog and its reply.
With the second change, a complete local run with the same external fixtures
passed 1,646 tests with four disabled. Its log is
`runtime/full-suite-town-selection-cleanup-2026-11-15.log` under the same
build root.
This is local evidence, not hosted CI or a complete audit of asynchronous
selection and cast acknowledgements.

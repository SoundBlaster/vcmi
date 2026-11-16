# Request acknowledgement review

The review correctly notes a change broader than toroidal maps: the server now
sends the visitor's boolean as `PackageApplied.result` for handled requests.
At baseline `30e9207`, a request blocked by an activity already received
`false`, but other rejected handler results were followed by `true`. The
branch changes that second case for bounded and toroidal games. This avoids
reporting a failed EndTurn as successful to AI, but its global compatibility
impact remains broader than the tested cases.

The boolean is not a transaction receipt. `visitMoveHero` applies steps in
order, stops on failure, and can report `false` after earlier legal steps have
already changed position and movement points. A blocking activity may instead
stop a request after an accepted prefix and return `true`. Other multi-item
handlers can also have partial effects. Consumers must observe authoritative
state; neither result alone proves all-or-nothing execution.

`HeroMovementTest.MovementAcknowledgementReportsRejectedAndAcceptedRequests`
now additionally sends a bounded-map two-step request whose first step is
legal and second enters rock. The server sends `false`, while the first step
and its movement cost remain applied. Its earlier assertions still check a
single rejected step and a successful step. This is a local server fixture,
not a complete audit of every visitor, client or AI consumer.

## Dialog acknowledgement consumers

`CPlayerInterface::requestRealized` calls the movement controller on a
`QuestionAnswer` acknowledgement without checking its result. The controller
clears its wait for that answer and can resume a movement sequence; this source
inspection does not establish a reproducible player-facing failure.

At the time of review, Nullkiller2 took the result into account: `AIStatus::receivedAnswerConfirmation`
removes a question on success but keeps it on rejection. `waitTillFree` waits
while that question remains, and the rejection branch has a TODO for a safe
retry. The same handling exists at baseline `30e9207`, although this branch
reports more visitor failures as rejected acknowledgements. The focused
`Nullkiller2_AIGatewayDialog.rejectedAnswerKeepsQuestionPendingUntilAnotherAnswerSucceeds`
test exercises the state transition with a synthetic rejected acknowledgement
and a separately supplied successful retry; it does not reproduce a normal
gameplay rejection or implement an automatic retry.

Simply clearing the question would be unsafe: `answerQuestion` can reject an
invalid or missing answer while the activity still requires a valid one. A
generic recovery needs a reason-aware protocol or a separately justified
recovery for each dialog type, along with checks for answer and event ordering.
The current boolean alone does not distinguish these situations. This remains
an open compatibility and AI-liveness issue, not a demonstrated toroidal-only
failure.

The new source comment and test clarify existing branch behavior; they do not
introduce a new protocol field, rollback, exactly-once delivery, or a rule that
false always means no change. Further review of AI retry and partial-result
handling is needed before treating this as a general-purpose agent contract.

## Server-confirmed question closure

Nullkiller now consumes the server's `QuestionResolved` notification. A
previously rejected answer remains pending until the question is explicitly
reported closed or a later answer succeeds. When resolution arrives while an
answer request is in flight, the AI waits for that request's acknowledgement
before releasing the question. It ignores resolution broadcasts for other
questions and tolerates an asynchronous answer sent after resolution. No
answer is retried automatically. Questions without a resolution notification
and genuinely invalid answers may still need a reason-aware recovery.

The client now registers a request with its player interface before sending it
to the network or the prediction path, preventing a reply on another thread
from overtaking Nullkiller's registration. This order change is based on code
inspection; a naturally occurring race was not reproduced. Unrecognized answer
acknowledgements are logged rather than asserting.

The synthetic `resolvedQuestionClearsRejectedAnswerWithoutAffectingOtherQuestions`
test failed before the change: the server-resolution callback left both
questions pending. Tests now cover both resolution/acknowledgement orders,
multiple in-flight rejected answers, an unrelated resolution broadcast, and a late
answer. Two additional tests use the real in-process server handler: a bounded
hero level-up delivers resolution before a successful acknowledgement, and a
toroidal hero level-up rejects an invalid answer, then a separately supplied
valid answer resolves the activity and clears the AI's pending question. The
adapter forwards server events to the gateway; it is not TCP or a full AI turn.
Eleven related tests passed five repeated runs on macOS arm64.

The complete local suite passed 1,661 tests with four disabled, using the
prototype VMAP/save and the Complete H3M; log
`runtime/full-suite-ai-question-resolution-2026-11-15.log` is on TB5. That
run used the same implementation before the rejected-answer server test was
switched from a bounded to a toroidal fixture; the latter version passed in
the repeated focused run. GUI and remote-network scheduling remain unverified.

On macOS arm64, `vcmitest` and `vcmiserver` rebuilt. The expanded bounded
acknowledgement test passed. The preceding full run at `e46d07a` passed 1,500
tests from 226 suites with four disabled; this step changes a source comment,
adds assertions to an existing test and updates documentation, rather than
changing the packet handling code.

## Activity-blocked movement and the human client

On 2026-11-16, a focused bounded-map server fixture installed a synthetic
activity that blocks movement, then submitted a `MoveHero` request through
`handleReceivedPack`. The server returned `PackageApplied.result=false`,
emitted no `TryMoveHero` through the test server's apply path, and left the
hero at the origin. This complements the earlier partial-movement test; both
passed on macOS arm64 using `6aa6e57` plus this test. XML is
`runtime/activity-blocked-movement-2026-11-16.xml` under the TB5 build root.

Source inspection shows `CPlayerInterface::requestRealized` ignores that
result and calls `onMoveHeroApplied`; if movement is active, a path remains,
no dialog is busy, and the next node is in the current turn, that method can
send another movement request. The fixture does not run the graphical client
or establish that those conditions occur together in normal play, so a live
retry loop is a risk, not an observed one. A negative acknowledgement can
also follow a partially applied batch. Any fix must stop or reconcile the
affected route using authoritative state and associate the acknowledgement
with its request; treating every unrelated failed movement reply as belonging
to the current hero could interrupt another route.

## Correlating human route acknowledgements

The human movement controller now registers only `MoveHero` requests sent
from its own route dispatch, for its active hero. Registration happens through
`requestSent` before the client sends the packet. A matching negative
acknowledgement aborts further automatic route movement without rolling back
any state or path updates already received. A matching positive response
continues the existing movement flow. An acknowledgement outside that route,
including an old or duplicate ID, is ignored. This applies on bounded and
toroidal maps. Question-answer acknowledgement handling is unchanged.

On macOS arm64, `vcmiclient` and `vcmitest` built from `676f572` plus these
changes. Three focused tests passed: request registration/consumption with
other heroes, unrelated sends and old replies; activity-blocked server
movement; and a server batch with an accepted prefix and rejected suffix.
XML: `runtime/movement-request-tracking-final-2026-11-16.xml` on TB5.
An earlier version also passed 67 movement and artifact tests; the final
registration-scope change was then checked by the three focused tests.
The configured full suite on the final change ran 2,100 tests: 2,086 passed,
14 external-fixture cases were skipped and three additional tests were
disabled. Log and XML: `runtime/full-suite-movement-ack-2026-11-16.{log,xml}`
under the TB5 build root. This suite does not execute the graphical controller.

The final client opened the isolated Earth seam preview with a human player.
Two diagnostic GUI clicks requested a three-step westbound path. Observation
reported `(0,425) → (251,425) → (250,425) → (249,425)`, with movement points
`1560 → 1260`; the client was force-stopped afterwards. The observation is
`runtime/earth-move-ack-qa-2026-11-16/observe-final.jsonl` on TB5. This is a
positive graphical route, not a live rejected acknowledgement or a complete
multiplayer test. A real GUI rejection, unrelated concurrent commands and
remote network timing remain unverified.

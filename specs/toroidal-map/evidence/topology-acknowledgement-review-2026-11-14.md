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
multiplayer test. A natural GUI rejection, unrelated concurrent commands and
remote network timing remain unverified.

## Graphical rejection and manual retry

On 2026-11-16, the client and server based on `f44cd5a` were built with a
temporary, opt-in server instrumentation that returned a negative
`PackageApplied` for the first `MoveHero`, before applying any movement. In
an isolated macOS arm64 Earth seam-preview profile, two diagnostic clicks
selected a three-step westbound route. The server log records request 6 and
the injected rejection at `05:01:33.136`; the observed hero remained at
`(0,425,0)` with 1,500 movement points. No further movement request appeared
before the next user-equivalent click at `05:01:56.260` (about 23 seconds).
That click resumed the route: requests 7–9 moved the hero through
`(251,425,0)` and `(250,425,0)` to `(249,425,0)`, ending with 1,200 points.

This exercised the real graphical movement controller and an injected reply,
not a naturally occurring blocking activity, a partially applied batch,
another concurrent hero, remote networking or visual smoothness. The client
was force-stopped. The test-only server source was removed, and client and
server were rebuilt; neither resulting binary contains the injection marker.
The observation and copied client/server log are
`runtime/earth-move-ack-qa-2026-11-16/observe-reject.jsonl` and
`runtime/earth-move-ack-qa-2026-11-16/client-reject-verification-2026-11-16.log`
on TB5.

## Teleport question acknowledgement correlation

The human movement controller formerly released its teleport wait on any
`QuestionAnswer` acknowledgement, ignoring both its request ID and result.
An unrelated response could therefore release the wait early, and a rejected
teleport answer was treated as if it had succeeded. These are code-level
risks; neither was previously reproduced as a natural gameplay failure.

The controller records the teleport question and sent request ID. Unrelated,
stale and duplicate acknowledgements do not release the wait. A rejected
answer stops the route without undoing server-applied effects. For the current
toroidal protocol, an accepted answer also waits for the matching resolution
and continues only if teleportation of that hero was observed. Bounded maps
and older negotiated peers use the conservative fallback described below.
The controller does not retry an answer or resolve an activity that stays open.

On macOS arm64, the client, server and test binary built from `b96c4bc` with
the client change later committed as `e26c744`. Seventeen focused
tests passed, including the new tracker checks for unrelated questions,
stale and duplicate requests, accepted and rejected replies, existing
movement acknowledgement tests, four whirlpool tests and nine AI dialog
tests. The tracker test verifies the decision state, not a complete GUI route.

In an isolated graphical session with the earlier staged water-portal map,
the human hero entered the portal from `(239,430,0)` and emerged at
`(168,290,0)`, with movement `1500 → 1400`; this exercised an accepted
teleport answer. A second isolated run used a temporary server-only injection
to reject the first `QuestionAnswer` before applying it. The hero remained
at the entrance `(239,429,0)` with 1,400 points; the log recorded one
rejection and no subsequent movement or answer request during a five-second
observation. The server activity remained unresolved. These GUI runs used
one-step shortcut movement, so they do not establish behavior of a longer
route after a rejection or recovery from an open dialog. Logs and observation
streams are under `runtime/teleport-answer-{gui,reject-gui}-2026-11-16/`
on TB5. The temporary injection was removed and client and server rebuilt;
its marker is absent from both resulting binaries.

## Route continuing past a portal

On 2026-11-16, a separate 36×36 toroidal fixture was derived from the local
`toroid-demo-clean-2026-11-15.vmap`: the red hero was staged at `(0,3,0)`,
the nearby resource removed, and two same-channel monoliths added with
entrances `(0,2,0)` and `(7,3,0)`. The selected destination `(7,4,0)` was
visible and required a step after the portal. The generated local VMAP has
SHA-256 `8e505853477a3e47ef8dd87b2860a63273c911538aeb6530008ba215d7e269b1`.
It and the logs are under `runtime/teleport-long-route-fixture-2026-11-16/`
on TB5; it is a staged test map, not a balanced scenario.

With client/server implementation `e26c744`, two diagnostic GUI clicks on
the destination produced an actual route. The server trace records movement
to the first entrance, an accepted `QuestionAnswer`, teleport to the second
entrance and a subsequent normal step to `(7,4,0)`. Movement points changed
`1560 → 1360`. This confirms normal continuation after the matching answer.

The same route was repeated in a separate profile with the temporary
server-only injection rejecting the teleport `QuestionAnswer` before
applying it. The hero stopped at `(0,2,0)` with 1,460 points; during the next
five seconds the client sent no further `MoveHero` or `QuestionAnswer`
requests. The server-side teleport activity stayed open. Logs and observation
are under `runtime/teleport-long-route-reject-2026-11-16/` on TB5. This checks
an injected rejection in one GUI route; it does not reproduce a natural
rejection, prove recovery of the pending dialog or test remote-network timing.
The injection was removed and the ordinary client, server and tests rebuilt;
the injection marker is absent from the resulting client and server.

## Unrelated acknowledgement during a portal route

The same GUI route was repeated with a temporary server instrumentation that
sent a successful `QuestionAnswer` acknowledgement with unknown request ID
`10006` while the actual portal answer was request `6`, then delayed the real
answer by one second. The client received the unrelated packet and logged
the expected unknown-request warning. It sent no further `MoveHero` request
between the injected packet and release of the real answer. After the actual
answer the hero teleported and reached `(7,4,0)`, spending 200 points in
total. This exercises the real client with an artificial stale/unrelated
acknowledgement, not a naturally observed race between two live dialogs.

The isolated observation and client/server log are under
`runtime/teleport-unrelated-ack-2026-11-16/` on TB5. The temporary injection
and delay were removed from the server source and the client, server and test
binary were rebuilt; the injection marker is absent from client and server.

## An accepted answer may precede teleportation

The server's `submitReply` can accept an answer for an activity that is
temporarily covered by another activity. A focused gate test now sends a
real `QuestionAnswer` while a test activity covers the teleport dialog:
`PackageApplied` reports success while the hero is still at the entrance;
only finishing the covering activity moves the hero to the paired gate.
This establishes the server ordering in a staged test, not that it occurs
naturally during a normal game.

Earlier client change `fff0f3b` prevents the acknowledged answer alone from
continuing a route. It tracks the actual `TryMoveHero::TELEPORTATION` for the
same hero. When acknowledgement arrives first, it stops automatic movement
instead of issuing the next step prematurely; a later server teleport still
updates the game state. This is a conservative fallback: the previously
selected route does not resume automatically after that deferred teleport.
It does not solve a rejected answer or a server activity that never finishes.

On macOS arm64, 22 focused movement, gate, whirlpool and dialog tests passed.
On the staged graphical portal route, the ordinary ordering still reached
`(7,4,0)` with movement `1560 → 1360`. In a second isolated run, temporary
server instrumentation placed a synthetic activity above the portal, sent
the accepted acknowledgement, waited one second and finished the activity.
No further `MoveHero` was sent after acknowledgement; the server subsequently
teleported the hero to `(7,3,0)` with 1,460 points, where it remained for a
four-second observation. Logs and observations are under
`runtime/teleport-confirmed-positive-2026-11-16/` and
`runtime/teleport-buried-gui-2026-11-16/` on TB5. The instrumentation was
removed and the ordinary client, server and tests rebuilt; its marker is
absent from both resulting binaries. Remote timing and naturally occurring
buried portal activities remain unverified.

## Resuming a deferred teleport route

The current toroidal protocol sends `QuestionResolved` after the teleport
activity's completion callback, including when it produces no movement. The
client waits for both the matching accepted answer and resolution, in either
order, and resumes only after observing the hero's teleportation. With an
older negotiated peer, it retains the conservative stop-on-early-ack fallback.
Bounded maps do not receive the new server notification. The protocol version
marks this semantic guarantee; the save format and packet structure do not
change.

The server activity test confirms the deferred ordering. On macOS arm64, an
ordinary graphical portal route reached `(7,4,0)` with movement `1560 → 1360`.
In a second graphical run, temporary server instrumentation held the teleport
activity under a synthetic blocker for one second after acceptance. The client
sent no next movement while waiting and continued to `(7,4,0)` after the
teleport and resolution. Instrumentation was removed and the binaries rebuilt.
Logs are in `runtime/teleport-resolution-normal-2026-11-16/` and
`runtime/teleport-resolution-buried-2026-11-16/` on TB5. This is not proof of
a naturally occurring overlapping activity or remote-network timing.

Fifty-two focused movement, gate, AI dialog, connection and save tests passed;
three optional external-server cases were skipped in that run. Separately,
two current test listeners connected to the real previous framed server
`6bf2051`, negotiated its version, selected a toroidal map and exchanged a
chat message. That covers the lobby only, not mixed-version gameplay. The
isolated logs are in `runtime/teleport-protocol-old-server-qa2-2026-11-16/`.

A separate server activity test exercises a subterranean gate on a bounded
map. It verifies a successful hero teleport followed by the answer
acknowledgement and asserts that no additional question-resolution packet is
sent. The bounded test, toroidal deferred-activity test and two tracker tests
passed together on macOS arm64. This is not a mixed-version gameplay test.

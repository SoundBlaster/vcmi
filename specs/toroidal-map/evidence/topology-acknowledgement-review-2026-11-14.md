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

The new source comment and test clarify existing branch behavior; they do not
introduce a new protocol field, rollback, exactly-once delivery, or a rule that
false always means no change. Further review of AI retry and partial-result
handling is needed before treating this as a general-purpose agent contract.

On macOS arm64, `vcmitest` and `vcmiserver` rebuilt. The expanded bounded
acknowledgement test passed. The preceding full run at `e46d07a` passed 1,500
tests from 226 suites with four disabled; this step changes a source comment,
adds assertions to an existing test and updates documentation, rather than
changing the packet handling code.

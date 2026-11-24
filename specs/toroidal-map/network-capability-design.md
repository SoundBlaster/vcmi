# Experimental lobby capability design

Status: experimental in the fork. See the
[upstream integration plan](upstream-integration-plan-2026-11-24.md) for the
current verification scope.

The lobby greeting carries a serialization version. The fork marks the high
16 bits with `0x5452` and uses the low 16 bits for the layout number. The marker
identifies the fork's protocol family. It does not authenticate a peer.

The integrated server accepts upstream greetings at versions `942–943` for
bounded play. A fork client accepts an upstream acknowledgement in the same
range. An upstream server that responds with the lower of its own version
and the offered version can acknowledge the tagged fork greeting using its
ordinary version. An untagged greeting above `943` has an unknown layout and
is rejected. Tagged beta greetings from the old range `940–946` are
rejected in both directions. The fork's integrated layouts start at `10000`
and its current greeting offers `10007`.

The server requires a negotiated layout with framing from every connected
peer before starting a toroidal game. A current fork client also requires
framing when accepting a tagged server greeting, even if the server will
choose a bounded map. A matched connection continues to use its negotiated
layout throughout the game. See [save format](save-format-design.md) for
the separate policy for saved games.

## Earlier wire-layout gap

The existing packet reader detects truncated input and unread trailing bytes,
but these checks do not identify every mismatched layout. On 2026-11-15 the
disabled `GameConnectionTopologyTest.DISABLED_RejectsConflictingTopologyWireLayouts`
was run explicitly: a current-format toroidal lobby map packet was decoded
with the previous layout and returned `toroidal=false` without an exception.
The reverse mismatch did throw in this particular test. This does not by itself
show a normally negotiated session starting with mixed layouts; the existing
capability guard rejects the tested untagged peer before toroidal start.

A per-packet layout discriminator could provide an additional fail-closed
check, but changing the raw packet layout under an existing version would
break compatible peers. Any implementation needs a new negotiated capability
or version, explicit rejection of peers that lack it for toroidal sessions,
and bounded-session compatibility tests. The switch must account for the
current ordering: the server's `clientConnected` can announce player-join
messages before it announces the new client's own `LobbyClientConnected`
acknowledgement. A receiver cannot assume it has seen that acknowledgement
when decoding those earlier messages. Multiple existing clients also receive
the new client's acknowledgement. A transition design must specify exactly
which packets remain in the original framing for each connection and test
two or more peers, reconnect, legacy peers, malformed headers and actual TCP.
The subsequent framed implementation below addresses this gap.

## Framed layout

Status: implemented experimentally and verified within the scope of the
[framed protocol evidence](evidence/network-framed-packets-2026-11-15.md). The first byte
of each framed packet is `1`, followed by ASCII `TRF` and a four-byte unsigned
little-endian serialization version. A framed receiver checks all eight bytes
against its negotiated version before decoding the remaining payload. A legacy
receiver interprets the first byte as a null packet pointer and rejects it;
the mixed-layout regression must test both directions and ordinary matching
packets. The marker is a compatibility discriminator, not authentication.

A new serialization version gates this protocol. The initial greeting, its
own acknowledgement, and any messages sent before that acknowledgement use
the existing unframed layout. Once the server has sent the acknowledgement to
the new connection, it switches that connection to framing before sending its
next packet; a current client switches after validating its acknowledgement.
Clients that have already completed the handshake continue using their own
negotiated framing while observing a later peer's acknowledgement. Servers
must reject toroidal game start if any connected peer lacks the framed version;
the current client rejects older tagged servers that could start an unframed
toroidal game. A supported untagged upstream peer may still use a bounded session with its
unframed version.

Verification must cover a two-peer TCP lobby, messages before and after the
acknowledgement, mixed and malformed headers in both directions, matching
current and legacy packets, compatibility of bounded sessions, save/reload,
and the existing toroidal gameplay tests. The older local builds are intentionally not interoperable for toroidal
sessions. Full acceptance remains pending; the evidence record distinguishes
real TCP tests, local graphical checks, and unverified mixed-version cases.

## Request acknowledgement is not a transaction receipt

`PackageApplied.result` is a server handler result, not a guarantee that a
multi-step request either fully committed or had no effects. A failed movement
can have applied a legal prefix; an activity can also interrupt a path after
an accepted prefix. See the [acknowledgement review](evidence/topology-acknowledgement-review-2026-11-14.md)
for the tested scope and bounded-map compatibility implications. Any future
agent interface must define its own action outcome and state-observation
semantics rather than inferring atomicity from this boolean.

For `TOROIDAL_TELEPORT_RESOLUTION` peers, a toroidal server sends the existing
`QuestionResolved` packet after completing the teleport activity callback.
The client correlates it with both the answer acknowledgement and the actual
hero movement. This is a negotiated behavioral guarantee, not a new packet
layout or a save-format version. Older framed peers remain eligible for
toroidal connections; a new client talking to one falls back to stopping a
route when an acknowledgement precedes teleportation. Bounded games do not
receive this additional resolution. See the [test evidence](evidence/topology-acknowledgement-review-2026-11-14.md#resuming-a-deferred-teleport-route) for the covered and unverified cases.

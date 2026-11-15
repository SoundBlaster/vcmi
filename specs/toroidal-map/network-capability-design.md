# Experimental lobby capability design

Status: implemented experimentally in the local branch; not an upstream
protocol allocation. Real mixed-version multiplayer remains unverified.

The existing lobby greeting contains one numeric serialization version. Its
ordinal cannot establish that another build implements toroidal maps: an
unrelated future build can reuse the same ordinal. Before allowing a
toroidal session, the server needs an explicit capability indication from
every connected client. The client also needs to reject an ambiguous greeting
from an unmarked server at or beyond this branch's toroidal version.

The proposed compatibility envelope uses the high 16 bits of the existing
greeting version as the experimental tag `0x5452`, and the low 16 bits as
the ordinary serialization version. The tag itself changes no packet fields. A separate negotiated version now
enables per-packet framing; file save versions remain independent.
The tag is a branch-local protocol marker, not a security credential or an
upstream-reserved namespace. The approach assumes ordinary upstream version
ordinals remain below 65,536 and requires upstream coordination before
release.

For a tagged greeting, the local server negotiates the ordinary version and
echoes the tag only if that version supports toroidal maps. For an untagged
greeting, it caps negotiation at the last version before the toroidal header
and records no toroidal capability. The toroidal start guard requires both
the tag and the necessary ordinary version. Bounded sessions continue to use
the negotiated legacy layout. A local client accepts an untagged server below
the toroidal version, but disconnects from an untagged server at or beyond
that version because its layout is ambiguous. The current client also rejects
an older tagged experimental server whose version predates framed packets, even for a bounded session: the greeting does not identify the map.
The current server may negotiate such a client into a bounded lobby, but its
toroidal start guard requires the framed-packet version from every peer.
A previous upstream server's
existing `min(clientVersion, serverVersion)` greeting negotiation responds to
the tagged value with its own untagged, lower ordinary version.

This is not authentication: a modified peer can claim the tag without
implementing the behavior. The separate [save marker](save-format-design.md)
protects newly written branch saves but leaves untagged prototype saves
ambiguous. New toroidal VMAPs now carry a separate branch-local major-version
marker; earlier version-4 prototypes remain ambiguous and neither marker is
an upstream allocation. This does not prove interoperability with arbitrary
future forks. Packet-level tests,
server preparation tests and real multi-peer sessions are separate evidence
requirements.

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
toroidal game. An untagged older peer may still use a bounded session with the
prior unframed version.

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

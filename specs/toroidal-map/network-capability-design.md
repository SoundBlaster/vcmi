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
the ordinary serialization version. This changes no packet fields and does
not change the serialization version used for subsequent packets or saves.
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
an older tagged experimental server whose version predates replicated movement
layers, even for a bounded session: the greeting does not identify the map.
The current server may negotiate such a client into a bounded lobby, but its
toroidal start guard requires the replicated-layer version from every peer.
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

## Request acknowledgement is not a transaction receipt

`PackageApplied.result` is a server handler result, not a guarantee that a
multi-step request either fully committed or had no effects. A failed movement
can have applied a legal prefix; an activity can also interrupt a path after
an accepted prefix. See the [acknowledgement review](evidence/topology-acknowledgement-review-2026-11-14.md)
for the tested scope and bounded-map compatibility implications. Any future
agent interface must define its own action outcome and state-observation
semantics rather than inferring atomicity from this boolean.

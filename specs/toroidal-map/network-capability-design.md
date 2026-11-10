# Experimental lobby capability design

Status: proposed for the local branch; not an upstream protocol allocation.

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
that version because its layout is ambiguous. A previous upstream server's
existing `min(clientVersion, serverVersion)` greeting negotiation responds to
the tagged value with its own untagged, lower ordinary version.

This is not authentication: a modified peer can claim the tag without
implementing the behavior. It also does not resolve collisions in saved-game
or VMAP version allocations, or prove interoperability with arbitrary future
forks. Packet-level tests, actual server start-guard tests and real multi-peer
sessions are separate evidence requirements.

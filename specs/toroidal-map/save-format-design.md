# Experimental save format marker

Status: implemented in the local branch; this is not an upstream allocation.

The branch-local `TOROIDAL_MAP` serialization ordinal can collide with a
future upstream ordinal that describes a different binary layout. Previously
written experimental saves have no distinguishing marker, so that ambiguity
cannot be resolved from their headers without breaking their readability.

For new saves, reserve the branch-local high-word marker `0x5452` in the
on-disk version for toroidal games. A reader in this branch unwraps the marker
before selecting the binary layout; an older unmodified reader sees a version
newer than it supports and rejects a full load. The marker is a compatibility
discriminator, not authentication, an upstream reservation, or a promise of
future interoperability. Unknown tagged versions must fail closed on full
load while preserving the existing metadata-only behavior for newer saves.

The upstream integration writes bounded saves at upstream layout `943`.
Saves requiring fork state use tagged layout `10007`, which includes explicit
object-template approach directions. A bounded map with unindexed shown
objects also needs the tagged layout to preserve those objects.

The integration retires tagged beta layouts `940–946`. Full loading rejects
those markers and untagged numbers above the supported upstream layout.
Metadata-only access still supports newer layouts, subject to the version
marker and chunk checks. Ordinary upstream saves keep their original
ordinals. Tagged toroidal VMAPs have their own version and are unaffected by
the savegame renumbering.

Untagged prototype saves can collide with upstream numbers. The known
prototype uses `940`, which upstream uses for allowed map difficulties. The
version reader assigns that number the upstream meaning. The existing lobby
loader requires the map-header and scenario-options chunk to decode fully
before offering the save for play. This does not establish a discriminator
for arbitrary untagged files. Preserve the original files and use the
matching older build if access is needed.

The verification and its limits are recorded in
[save marker evidence](evidence/topology-save-marker-2026-11-10.md). These
checks do not prove that unrelated forks or future upstream versions will
recognize the marker.

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

For newly saved bounded games, write the previous upstream serialization
version and use that version for both binary chunks, so the extra topology
field is absent. Reading previously written branch saves remains supported;
an untagged save with the experimental ordinal is inherently ambiguous and
cannot be guaranteed safe against a future collision. No existing save is
rewritten or deleted. Before upstream release, this mechanism and the VMAP
major-version allocation require coordination with maintainers.

The verification and its limits are recorded in
[save marker evidence](evidence/topology-save-marker-2026-11-10.md). These
checks do not prove that unrelated forks or future upstream versions will
recognize the marker.

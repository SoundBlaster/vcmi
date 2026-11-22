# Platform checkout reconciliation, 2026-11-23

This is a point-in-time preparation for a later Platform rollout, not a
production deployment. The live `/srv/0al/platform` checkout remained at
`cda4ee4b082c55c008308c63fc505c36a83f631a` throughout the investigation.
The running Platform and ingress image references match the evidence image
lock whose `source_commit` is `d1e1d1f9e8df644aaef58ab4c88ad4ecb29c4d20`;
that differs from the checkout HEAD. The production deploy helper requires the
selected image lock and clean checkout HEAD to agree, so image and checkout
provenance must be reconciled together, not by advancing HEAD alone.

The live checkout contained:

- A modified Caddyfile whose SHA-256 is
  `8c9f0724c8733e15bc498f7b2e461ab3bbdcc19d81fef32bb47beea9680ad427`.
  It is mounted into the running ingress container and is identical to
  Platform commit `ecd64ddcbd3572976ae6be58043f6abb535fd4d3`.
- An untracked `docker-compose.normlab-inference.example.yml` whose SHA-256 is
  `65e1f2b51ef98bb77c663e63f64d765aceedbcaa21542fb21718355c7748d55e`.
  Three running NormLab containers identify it as a Compose source file. Its
  bytes are identical to the tracked file in the Platform commit above.
- An untracked `deploy/hosted-managed/Caddyfile.before-vcmi-20261117` backup.

The three files were copied with metadata, compared byte-for-byte, and checked
against `SHA256SUMS` in the root-only server directory
`/srv/0al/backups/platform-checkout-reconcile-20261123/`. That directory also
contains the original HEAD, tracked diff and status, plus a disposable Git
rehearsal with retained stashes. No data was removed from the live checkout.

In the isolated rehearsal, a direct checkout of the Platform commit above was
refused because the modified Caddyfile and untracked Compose file would be
overwritten. After saving the changes with an untracked-inclusive stash, the
checkout was clean and both deployed configurations matched their saved copies.
A held file descriptor established that the checkout replaced the Caddyfile
inode: an existing file bind mount can continue pointing to the previous inode.
The rehearsal initially used a shared clone without the source repository's
partial-clone promisor configuration; after restoring that configuration and
missing files, a complete repeat succeeded. This is not an end-to-end test of
the production deployment helper or of rollback.

For a future controlled deployment, first select and validate an image lock
for the intended published Platform revision and preserve the current image
lock, checkout state and service rollback evidence. Verify that the chosen
revision contains the active Heroes and NormLab configuration, and check for
new local changes before applying any preservation step. The existing checkout
helper requires a clean tree; saving local changes temporarily must account
for the active NormLab Compose path. Recreate or otherwise safely rebind the
ingress during the coordinated rollout so it reads the new Caddyfile inode.
Use Platform's bounded deployment and rollback procedure, then check both
services, authentication, `/heroes3/`, the painter API and actual runtime
state. Do not blindly apply a saved stash on top of the target revision.

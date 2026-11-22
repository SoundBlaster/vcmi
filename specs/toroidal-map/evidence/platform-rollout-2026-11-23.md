# Platform runtime alignment, 2026-11-23

The Platform checkout, active release image lock and running image references
were aligned to source commit `ecd64ddcbd3572976ae6be58043f6abb535fd4d3`.
The live checkout is clean. Earlier dirty checkout findings are retained in
[the reconciliation record](platform-checkout-reconciliation-2026-11-23.md).

## Image lock provenance

The original image lock and multi-platform Platform/ingress images were
published by [GitHub Actions run 45076415101](https://github.com/0al-spec/Platform/actions/runs/45076415101)
for the exact source commit above. The workflow resolved a newer PostgreSQL
digest than the two running databases. Routine Platform deployment explicitly
rejects a PostgreSQL image change; no database migration was requested.

An operator-derived lock retained the published Platform and ingress images
and the previously deployed PostgreSQL image. It records hashes of both input
locks, the workflow run and the reason for retaining the database image. The
original CI artifact was preserved independently. The derived lock passed
Platform's image-lock validation, release-only environment comparison and
production preflight. It was installed as
`/srv/0al/evidence/hosted-managed-image-lock.json`, SHA-256
`83ce2e8c8660e50eee88623224a700c024f5e53408048470ad1a69c106c3e58f`.
This is an operator-derived lock, not the byte-identical CI artifact.

## Backup and deployment

Before the rollout, the bounded backup cycle used the existing
`bounded-product-dry-run` profile, quiesced mutable boundaries, backed up
both databases and workspace artifacts, and passed an isolated restore smoke.
Backup `upgrade-20261123-preflight` is under `/srv/0al/backups/`. Previous
environment, image lock, Caddyfile and checkout evidence remain in the
root-only `/srv/0al/backups/platform-runtime-upgrade-20261123/`; the earlier
three local checkout files remain backed up in
`/srv/0al/backups/platform-checkout-reconcile-20261123/`. A Git stash preserves
the original checkout changes. No production database restore was performed.

The bounded deployment report records a clean matching checkout, successful
preflight and Compose validation, drained queue before and after quiescing,
unchanged PostgreSQL image and operation allowlist, and five healthy services.
The resulting deployment report is at
`/srv/0al/evidence/hosted-managed-deployment.json`; the previous report was
retained in the runtime-upgrade backup directory. No managed operation was
enqueued by the rollout.

## Post-deployment checks

- Both Platform runtime image refs and both PostgreSQL container image refs
  matched the active derived lock. The ingress Caddyfile bind mount and its
  host path had the same inode after recreation.
- Queue audit reported no active jobs or locks, with 56 events before and after
  the rollout; this is a count check, not a content comparison.
- The Heroes landing and terrain editor returned HTTP 200. Two small test maps
  generated through HTTPS were valid toroidal VMAP archives; forged forwarding
  headers were not accepted as the client address.
- Platform and NormLab returned HTTP 401 for unauthenticated requests, and
  NormLab's three containers were healthy. Authenticated NormLab workflows
  and a multiplayer Heroes match were not exercised.

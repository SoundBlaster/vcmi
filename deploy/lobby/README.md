# Standalone VCMI lobby

This Compose project runs the lobby and its optional traffic relay, not a game
match server. The TCP listener is public on port 3031; Docker-published ports
may bypass UFW. The HTTP API listens inside the container on loopback only.
SQLite persists in the `lobby-data` volume. Back it up before upgrades; review
schema changes rather than replacing an existing database.

The pilot uses the official upstream VCMI CI artifact from run `43575525922`,
commit `948333932fdf91abda780906bf2d18eec9aa0fc1`, artifact ID `14106212890`,
SHA-256 `a14a471cfe64d6ce559624cafcbd8561143c4c971f050ca48b1c09d63fd1e513`.
Obtain it from the CI artifact endpoint, verify the hash, and place it beside
the Dockerfile as `vcmi-lobby.deb`. The package is not committed here. This
upstream development build is experimental and requires client compatibility
testing with the fork.

Run `docker compose config`, `docker compose build`, and `docker compose up -d`.
Inspect with `docker compose ps` and `docker compose logs`. Avoid `down -v`:
it deletes the lobby database. The image runs the lobby as a non-root user;
the entrypoint initializes the database only when absent.

Clients use settings `lobby.hostname` and `lobby.port`; the protocol is TCP,
not HTTP, so a URL path such as `/heroes3` cannot select the lobby. A DNS
hostname pointing at the VPS can provide a readable address on port 3031.
The separate `website` service serves `site/` at `/heroes3/` and joins the
existing Platform ingress network. Its `nginx.conf` sends
`Cache-Control: no-cache` under `/heroes3/`, because pages and maps are
replaced under the same names. `Caddyfile.fragment` records the HTTPS
route installed in the Platform Caddyfile. The Platform deployment owns that
Caddyfile: a subsequent Platform deployment can replace the route until it is
incorporated in Platform's own configuration. Keep game assets and secrets out
of `site/`.
Confirm lobby login, room creation, relay and an actual match with two clients;
an open TCP port by itself is not an end-to-end test.

### Deployment inventory and recovery

The landing page advertises preview `toroid-preview-da6c93a` for macOS arm64
and Windows x64. These release binaries predate later commits such as the
terrain painter layout; changes on the site do not update installed clients.
The downloadable `.vmap` files are server assets, not tracked in this Git
checkout. Back them up separately before replacing the site and verify their
hashes and mod requirements. The original generated-world evidence and the
different published World map are distinguished in
[`world-map-from-picture-2026-11-22.md`](../../specs/toroidal-map/evidence/world-map-from-picture-2026-11-22.md).

On 2026-11-22, the live Platform Caddyfile contained an uncommitted `/heroes3`
route. The fragment in this repository describes the integration but is not a
substitute for committing it in Platform's deployment configuration. Preserve
the other local changes in that checkout and verify HTTPS after that separate
change is integrated. Do not overwrite the running configuration blindly.

## Terrain painter

`site/editor/` is a static page that turns pictures and drawings into a
terrain grid. The `mapbuilder` service receives that grid at `/heroes3/api/map`
and runs `vcmimapbuilder` (target `ENABLE_MAPBUILDER`), which paints the
terrain with the engine's transition rules, places obstacles and returns the
`.vmap`. nginx forwards `/heroes3/api/` to it.

The engine reads text tables and object masks of Heroes III while loading.
`mapbuilder/data/` holds archives made by `mapbuilder/stub_lod.py` from
`H3bitmap.lod`, `H3sprite.lod`, `H3ab_bmp.lod` and `H3ab_spr.lod`: every entry
keeps its name, only `.TXT` and `.MSK` entries keep their content (about 1.3 MB
in total). They are not committed here.

The server shares its CPU with other services, so compile in a limited
container instead of `docker build`, then build the runtime image from `out/`:

    docker run --rm --cpus 1 --memory 2g -e JOBS=1 -v $PWD/src:/src \
        -v $PWD/out:/out -v $PWD/build:/build -v $PWD/build.sh:/build.sh:ro \
        ubuntu:24.04 sh /build.sh

`src/` is a copy of this repository without client, editor, launcher and test
sources.

### Public API hardening needed

The current nginx configuration forwards the incoming `X-Forwarded-For` value
as `X-Real-IP`, which mapbuilder uses as its per-client rate-limit key. The
website's directly published HTTP port allows callers to supply that header,
so the limit is not a reliable abuse boundary. Requests are also read before
the limit is checked, and the threaded server has no overall connection bound.
Before relying on this service for broader public use, restrict direct access
to the trusted ingress, set client identity only from a verified proxy chain,
bound request concurrency and timeouts, and test both the ingress and direct
access paths. The current `health` response only shows that the HTTP service
answered; it does not test map generation, the lobby or a multiplayer game.

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
Confirm lobby login, room creation, relay and an actual match with two clients;
an open TCP port by itself is not an end-to-end test.

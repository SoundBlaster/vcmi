# Current preview and corner crossing

The macOS arm64 client and server were built from clean revision `155b38b`
under the checkout-specific TB5 build root. A separate app was assembled as
`runtime/VCMI Toroid Preview 155b38b.app`, retaining the earlier preview's
bundle identifier and the user's authorized Apple Development signing identity.
Its client, server and engine library were replaced with the new build, and
`codesign --verify --deep --strict` passed. Some other dependencies still
resolve through the TB5 build toolchain, so this is a local development
preview, not a portable or notarized distribution.

The client was invoked directly from `Contents/MacOS` with a private temporary
log, command socket and observation file. The window showed version
`1.8.0.155b38b` and opened `Toroid Seam Demo`, the clean 36×36 toroidal VMAP
with SHA-256
`9a945a811529805ac53c28352027e01129446f434847497b0512d770600e1c0a`.
The map selector warned that the map is experimental and AI support remains
unverified. The observation channel reported the red hero at `(0,0,0)` with
1,560 movement and 20,000 gold. One northwest command collected 1,000 gold
without moving the hero; a second moved it to `(35,35,0)` with 1,278 movement.
The client exited with code 0, the embedded server logged a clean shutdown and
port 3030 was not listening afterwards. No manual save was made; the game may
have created scenario directories. Existing other VCMI processes were left
alone. The client log and observations are in
`runtime/gui-preview-155b38b-2026-11-15/` on TB5.

SHA-256 of the signed client, server and engine library, respectively:

```text
72e5143d1c331973391da31a74e802f95c67a7f86366be295a06deac524d4ef1
e098d26e86da3ce1b5b029d2e262c919f2fe502472ea33b94f70977ddc6fe36d
0dd8cd49af60e7a9c540890bfe99199e5eb87fb5beab5e9979ca68ea1cb60309
```

This run does not check an AI turn, remote multiplayer, smooth scrolling or
animation. The client log includes unrelated warnings for unsupported HotA
maps during scenario-list scanning. The screenshot was observed but not
retained. Launch Services and an interactive Terminal launch were not tested
for this bundle; earlier attempts through Launch Services stalled while
opening TB5-backed paths.

The directly invoked equivalent is shown below. A successful invocation from
the test environment does not establish that another launching application has
the same macOS permissions.

```sh
preview_log_dir="$(mktemp -d /tmp/vcmi-toroid-preview.XXXXXX)"
cd '/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/VCMI Toroid Preview 155b38b.app/Contents/MacOS'
./vcmiclient --logLocation "$preview_log_dir"
```

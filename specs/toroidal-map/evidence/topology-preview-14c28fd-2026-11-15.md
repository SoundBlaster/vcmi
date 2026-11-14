# Current preview and road fixture

The macOS arm64 client and server built from `14c28fd` are packaged in
`runtime/VCMI Toroid Preview 14c28fd.app` under the TB5 VCMI build root. The
bundle is signed with the authorized local Apple Development identity and
passes `codesign --verify --deep --strict`. It retains the identifier and
designated requirement of the earlier `496d90b` preview, while the earlier
bundle remains intact and also passes signature verification. This is an
experimental local test build, not a distributed or notarized release.

Directly invoking its signed client from `Contents/MacOS` with a private
diagnostic socket and local log started `1.8.0.14c28fd`. A new single-player
game opened on `Toroid Road Axes`; the observation channel reported a 36×36
toroidal map and the hero at `(0,0,0)` with 1,560 movement points and
20,000 gold. The first northwest command collected 1,000 gold and left the
hero in place with 1,419 movement points; the second moved the hero across
the corner to `(35,35,0)` with 1,278 points. The game closed and port 3030
was not listening afterwards. The game created a new save directory; the
session was not manually saved. Client log and observations are in
`runtime/gui-preview-14c28fd/` on the same build root.

The earlier [road observation](topology-visual-seams-2026-11-10.md) used
`496d90b`; this run confirms the new bundle starts and the corner interaction
works, not the visual continuity of every road or the smoothness of scrolling.
Launching the new bundle through Finder or Launch Services was not tested;
identical signing identity alone does not prove future macOS disk access.

For local direct invocation from a Terminal that has access to the data:

```sh
mkdir -p /tmp/vcmi-toroid-preview-log
cd '/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/VCMI Toroid Preview 14c28fd.app/Contents/MacOS'
./vcmiclient --logLocation /tmp/vcmi-toroid-preview-log
```

The test used a different local temporary directory for the log and socket;
macOS permissions may differ for an interactive Terminal session.

Signed binary SHA-256 values:

```text
ca2138589bcd466807b062118365dd4f44857c38d24d984297b167b30b9d4098  vcmiclient
456e5e41f601bf0dc2ecfac02f4bf9cc0992ca35a7ff5a6334a4c91700b9794c  vcmiserver
92a32f23410a17ea435f736a7a140a0682bc336be450a833e395d7213bfc80e5  libvcmi.dylib
```

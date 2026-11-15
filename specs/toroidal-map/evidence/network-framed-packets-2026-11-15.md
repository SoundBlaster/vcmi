# Framed network packets and connection negotiation

The previously disabled mismatched-layout regression demonstrated that a raw
current toroidal lobby-map packet could be read using the preceding layout as
`toroidal=false` without an exception. The reverse direction threw in that
case. This is a packet-level ambiguity, not evidence that the existing lobby
guard permitted an ordinary mixed-layout toroidal game.

The experimental branch now negotiates a new serialization version and frames
subsequent packets with eight bytes: `01 54 52 46` and the negotiated version
as a little-endian 32-bit integer. The receiver checks the complete marker and
version before payload decoding. An old raw reader interprets the first byte
as a null packet and rejects it. The greeting and packets through the new
peer's acknowledgement remain raw on that connection, including join messages
sent before the acknowledgement. The server enables framing for the new peer
after sending its acknowledgement; the client does so after accepting it.
Existing connections retain their own modes while observing that acknowledgement.
A toroidal start requires the framed version from every peer. The new client
rejects earlier tagged experimental servers, including for bounded sessions;
older untagged bounded sessions remain on their negotiated legacy format. Both
bounded and toroidal save files retain their earlier file versions: framing is
only a network change. This marker is a compatibility discriminator, not
peer authentication or general validation of hostile payloads.

## Automated verification

On macOS arm64, 34 focused tests passed, including matching and mismatched
layouts in both directions, truncated, corrupted and wrong-version frames,
old-version toroidal start rejection, multiple participants and pre-ack
messages. A loopback TCP test sends a bad frame from one client, verifies its
disconnection, then exchanges chat with the remaining host. The test server
and listeners run in the test process; it is not a remote-host test. A TCP
lobby and save reload also passed. Log: `runtime/framed-wire-tcp-tests-2026-11-15.log`
under the TB5 VCMI build root.

The complete suite passed 1,676 tests with three disabled; log:
`runtime/full-suite-framed-wire-final-2026-11-15.log`. The optional prototype
save, toroidal VMAP and legacy H3M inputs had unchanged SHA-256 values after
the run: `ad2a9df20f1995f62ffd215d62a6e8fa8cb4856443aa9302144f19d864d3ca73`,
`3fbc0a2cbbdfa22068e7e6a099a7b51e803d39b5fc0d0b3fd95155cc6eeb232d`, and
`cc23096e678a2e3bd9cf62445d7bba1828924222969b6ca5828d4805886fc162`,
respectively. The build completed with duplicate-library and absent Conan
search-path warnings.

## Local graphical client

A development-signed `VCMI Toroid Framed 6bf2051.app` was built with the
implementation changes on top of the named specification commit and verified
with `codesign --verify --deep --strict`. The signed client launched directly,
opened `Toroid Roads Demo`, and started a single-player game through its local
server. The map SHA-256 was
`b451f983719e345fed98cc13f6ac5bd5472a6b3b943a4bfd60c68c291213bc74`.
The observation stream recorded a resource visit followed by a northwest
corner crossing from `(0,0,0)` to `(35,35,0)`, with movement reduced from
1,560 to 1,278. The client and its server exited normally, and port 3030 was
not listening after exit. The session was not manually saved. Client log and
observations are in `runtime/gui-framed-6bf2051-2026-11-15/` on TB5.

Signed binary SHA-256 values:

```text
4317061d6912bcadee1b7fd5f636e1c5097aeb8fdcf84de9d1cd9b148e777157  vcmiclient
de7f9b3946bae850f3f1db03202b766f4d90e65e9dba2da65f551576e55bd44a  vcmiserver
5b9eacc55446460311fd7262b64b51ec0526867619200287331cbcdc70c05b12  libvcmi.dylib
```

The new protocol has not yet been exercised with two graphical clients, a
remote host, actual older network binaries, or a malformed server packet to
the graphical client. The local check does not establish smooth camera or
animation timing. The test build is not a distribution or notarized release.

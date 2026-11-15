# Current lobby codec against the original server

After the framed-packet change, an external macOS arm64 `vcmiserver` built
from the original `30e9207276f137658da5132705c05644bfe33aee` baseline was
started with an OS-assigned port. The test
`GameConnectionTopologyTest.ConnectsToExternalLegacyServerWithBoundedMap`
connected two current test listeners over `127.0.0.1`. Each sent the current
tagged greeting, accepted the untagged older negotiated version, selected a
bounded map in the old server's lobby, received a bounded map header and
exchanged a chat message with the other listener. The selected map URI matched
on both listeners. No framed packets were enabled for the old version.

The original server executable SHA-256 was
`e3aa2aa4248929eca2c3ea02b90dd1ec8526f69d3214348484f1dbc4442c0ecd`;
its bundle-local `libvcmi.dylib` SHA-256 was
`a12f1b546e042b47eb93f5aa48633c2de08856c9f525c6cf4270a476eb2e526b`.
The bounded map SHA-256 was
`c5bdd57645d7329f23225cda4491f9005ebffaa9b352e3b0da1d3d046aeb3f1c`.
The test executable was built from `55f3be2` plus the test in `dff78b0`.
The focused test passed; 15 related network tests passed in a separate run,
where the optional external-server test was skipped because no port was
supplied. Reports and server stdout are under the TB5 build root:
`runtime/external-old-server-probe-2026-11-15.{log,xml}`,
`runtime/external-old-server-related-2026-11-15.{log,xml}` and
`runtime/external-old-server-2026-11-15.log`.

The server listened on all IPv4 interfaces; the listeners connected over
loopback. The process exited after the listeners disconnected and its port
was no longer listening. This exercises the shared current codec and lobby
negotiation with a real old server, not the graphical client or a running
game. It does not cover other legacy versions, remote hosts, save/reload or
the inability of an old server to run a toroidal map.

The old server's `--help` invocation initialized and truncated the existing
default `VCMI_Server_log.txt` before the run. The later test run's engine log
was copied to `runtime/external-old-server-engine-log-2026-11-15.txt`; the
short post-help log was restored afterwards. The earlier log could not be
recovered from that file.

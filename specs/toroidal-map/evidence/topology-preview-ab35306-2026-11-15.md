# Local preview and corner crossing at `ab35306`

The macOS arm64 client, server and engine library were built from `ab35306`
and packed into a separate bundle at `runtime/VCMI Toroid Preview ab35306
local.app` under the checkout's TB5 build root. The existing preview helper
made client/server and library paths bundle-local and signed the copy with the
authorized Apple Development identity. It is a local development bundle, not
a notarized or portable distribution; the helper does not audit every bundled
program. Build warnings included duplicate libraries and absent Conan search
paths.

Launch Services started this bundle with a private `/tmp` log directory.
After about 30 seconds no game window was observed. The log had reached the
scan of `~/Library/Application Support/vcmi/Saves`, which resolves to TB5.
A process sample placed its main thread in `open` while constructing a
recursive directory iterator. This locates the wait but does not identify its
cause or establish that a macOS permission dialog was shown. The test process
was terminated; other existing VCMI processes were not touched.

The same client launched directly from `Contents/MacOS`, with a private
diagnostic socket and log directory, loaded the user's Complete resources.
The menu showed version `1.8.0.ab35306` and the selected `Toroid Seam Demo`;
the client log identified `toroid-demo-clean-2026-11-15.vmap`, SHA-256
`9a945a811529805ac53c28352027e01129446f434847497b0512d770600e1c0a`.
The observation channel reported a 36×36 toroidal map and the red hero at
`(0,0,0)`. Two northwest diagnostic shortcuts collected 1,000 gold and moved
the hero to `(35,35,0)`, with movement points `1560 → 1419 → 1278`. Blue was
configured for Nullkiller2; the test did not end the turn or exercise that AI.
The player exited without a manual save, and the client and its local server
closed normally. The game created a scenario save directory. Map-list scanning
reported separately installed HotA maps as unsupported.

After gameplay, only the bundle's display name was corrected and the bundle
was re-signed. Deep strict signature verification and a directly invoked
`--version` succeeded after that metadata change; gameplay was not rerun.
The final files have SHA-256 values:

```text
84495e6b843a3d382e659c39f966eb389d078b090a3dca1312545d19a250e484  vcmiclient
2afb955e70f7be74367d92e6f8d3d5361ec18640ac927204d6306565efc449ba  vcmiserver
131c0a52648b1f1cc76831bf23a0f0082e93f07338729722817f399070b7ca20  libvcmi.dylib
```

The client log, observation stream, Launch Services log and sample are under
`runtime/preview-ab35306-2026-11-15/` on TB5. No screenshots were retained.
This confirms a specific local game-state transition, not smooth animation,
an interactive Terminal launch, Finder launch, remote multiplayer, or full AI
support. A direct launch from the testing environment does not establish which
other applications have macOS access to the external volume.

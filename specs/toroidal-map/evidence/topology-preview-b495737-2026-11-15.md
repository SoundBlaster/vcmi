# Local preview and macOS launch checks

On macOS arm64, the client and server were rebuilt at `b495737` and packed
with the rebuilt engine library into a separate local
`runtime/VCMI Toroid Preview b495737.app` on the checkout's TB5 build root.
The bundle uses the previously authorized Apple Development identity and
passed deep strict signature verification. It retains the prior preview's
bundle identifier. This is a local development bundle: other search paths
still reference the TB5 toolchain, and it is not a notarized distribution.

The signed client was invoked directly with its working directory set to
`Contents/MacOS` and a fresh local temporary log directory. It loaded the
Complete resources and opened `Toroid Seam Demo` (map SHA-256
`9a945a811529805ac53c28352027e01129446f434847497b0512d770600e1c0a`). The
map selection screen identified the map as experimental, with AI support
unverified. The player observation reported a 36×36 toroidal map. Two
northwest commands collected 1,000 gold and moved the red hero from
`(0,0,0)` to `(35,35,0)`; movement points fell from 1,560 to 1,278. The
client exited with code 0, its embedded server logged a clean close, and no
listener remained on port 3030. No manual save was made. The test did not
end the turn, test the AI, compare all replicated state, or establish
continuous animation. Map-list scanning also logged unsupported HotA maps.

After the game session, the bundle's display name was corrected and the
bundle was re-signed. The directly invoked client opened the main menu under
the new name and exited with code 0. The gameplay session predates this
metadata and signature change; it used the same rebuilt game code and data.
The final bundle's client, server and engine SHA-256 values are respectively
`3b8d1e9e0ab6689df23b3966a2e32bc17f978498b2e8887285f5b4d514431ad2`,
`aaa02aa7c44c959a8a1db5e7fd377c8e14b4c33a37ce0075ae9736a567d3472c`, and
`cebcef838a027519300f25d05d0a45c0424951244c92684ad28cf053f5774a03`.

## Launch Services diagnostics

Launching the final bundle as a macOS application did not yield a window
within 30 seconds. A sample showed its main thread in a dyld file `open`,
before game code; the exact file and cause were not identified. The process
was stopped. A second bundle, `runtime/VCMI Toroid Preview b495737 bundled.app`,
was separately created with all 14 referenced non-system dylibs present in
the bundle and build-tree rpaths replaced by bundle-local rpaths. Its
signature verified and its directly invoked `--version` completed. Launch
Services still produced no window within 20 seconds, but its sample had
advanced into the game's default log-file `open` on the TB5-linked profile.

A third separate bundle, `runtime/VCMI Toroid Preview b495737 local-log.app`,
used a small signed launcher that changes into the app's MacOS directory and
passes a fresh `/tmp` log directory to the same client. It also passed deep
signature verification and its direct `--version` completed. Launch Services
still produced no window within 20 seconds; its local log reached the scan of
`~/Library/Application Support/vcmi/Saves`, and its sample showed a directory
iterator waiting in `open`. The game profile is symlinked to TB5. Neither the
samples nor these timeouts establish which authorization or filesystem
condition caused the waits. Both diagnostic processes were stopped, and the
existing other processes were left alone.

Logs, observations and the three samples are under
`runtime/gui-preview-b495737-2026-11-15/` on the TB5 build root. A manually
invoked Terminal session and a successful ordinary Finder launch remain
unverified. These checks do not establish portable packaging or resolve the
removable-volume access issue.

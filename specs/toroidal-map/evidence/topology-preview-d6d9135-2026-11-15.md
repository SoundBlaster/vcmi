# Current preview after AI question handling

On macOS arm64, `vcmitest`, `vcmiclient` and `vcmiserver` were built from
the clean `d6d9135` revision under the checkout-specific TB5 build root.
The client, server and engine library were packaged into the separate
`runtime/VCMI Toroid Preview d6d9135.app`, using the authorized local Apple
Development identity and the same bundle identifier as the earlier preview.
Its signature passed deep, strict verification. The prior preview was not
overwritten. This is a local development build, not a notarized release.
With the three external prototype and legacy fixtures configured, its
complete local test suite passed 1,661 tests with four disabled. The log is
`runtime/full-suite-preview-d6d9135-2026-11-15.log` on TB5; this is not
hosted CI.

Directly launching its client with a private diagnostic socket and local log
opened `1.8.0.d6d9135`. A new single-player game on the existing untagged
prototype `Toroid Seam Laboratory` reported a 36×36 toroidal map. Two
northwest commands collected 1,000 gold and moved red's hero from `(0,0,0)`
to `(35,35,0)`, leaving 1,278 movement points. Red ended the turn. The client
identified the blue opponent as Nullkiller2; the server applied blue hero
Cuthbert's diagonal movement from anchor `(18,35,0)` to `(17,0,0)` across
the south-north seam. Blue ended its turn and the server gave red the next
turn; the game screen then showed day two. The session ended without a manual
save; the game may create an autosave at turn boundaries.

No AI level-up or rejected question occurred in this session, so this launch
does not verify the new question-resolution path in a graphical game. The
server-backed tests for that path are described in the
[acknowledgement review](topology-acknowledgement-review-2026-11-14.md).
The screenshots were observed but not retained. The client log and
player-filtered observation trace are in
`runtime/gui-preview-d6d9135-2026-11-15/` on TB5.

SHA-256 values for the signed bundle binaries and the session evidence:

```text
2fcf812f5c950f67b33411581c61024c71bbcce32e72f45d7bff742e75f2e348  vcmiclient
0c42455a1308c04388152c4417ebe04109938944b77d7f03b6e126da2eea9b0e  vcmiserver
43237c5fb3937de6346f6edd28de7bd9076405230b080f8450e43e95a2370497  libvcmi.dylib
291f8aa3db3c78f063e1d4722984a8231cb3ae6fb8f40a1a1a737806e0fdf5fb  VCMI_Client_log.txt
5a8e306d901987e7bd061007f6ffee4682367dccec64499adc100c482d5d486e  observations.jsonl
```

Remote multiplayer and smoothness of scrolling or movement animation were
not checked in this session.

## macOS application launch attempt

Computer Use asked macOS to open the signed preview bundle by path. It
created the correct `vcmiclient` process, but no game window appeared within
30 seconds. A process sample showed the main thread blocked in `open` while
the default logger was being configured. The configured log directory
`~/Library/Logs/vcmi` is a symlink to this checkout's TB5 runtime. The sample
does not identify the precise path being opened.

A separate signed experimental bundle used a small launcher to execute the
same client with a new private `/tmp` log directory. It opened the local log
and progressed to `Loading filesystem .../vcmi/Saves`, but still produced no
window within 30 seconds. Its process sample showed a directory iterator
blocked in `open`; the user-data directory, including `Saves`, is on TB5.
The system log recorded All Files preflights but did not show a removable
volume prompt or decision for either launch. Neither observation establishes
why the filesystem call waited. The two test processes were stopped before
gameplay; the experimental launcher bundle and its binary were removed.

Logs and process samples are in `runtime/finder-preview-d6d9135-2026-11-15/`
under the TB5 build root. Ordinary Launch Services startup is not verified;
the direct launch with a local log remains the successful route demonstrated
above. An interactive Terminal invocation using a fresh private log directory
is:

```sh
preview_log_dir="$(mktemp -d /tmp/vcmi-toroid-preview.XXXXXX)"
cd '/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/runtime/VCMI Toroid Preview d6d9135.app/Contents/MacOS'
./vcmiclient --logLocation "$preview_log_dir"
```

The successful run was launched by the testing environment, not by an
interactive Terminal; the two processes may have different macOS permissions.
Computer Use refused to open Terminal for this check, so this command was not
verified through the Terminal application.

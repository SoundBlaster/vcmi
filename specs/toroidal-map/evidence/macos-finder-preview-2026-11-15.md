# Finder preview with separate client logs

The ordinary VCMI client defaults to `VCMI_Client_log.txt` and truncates it
on startup. An earlier VCMI process had this file open during the preview
check, so starting another default client could disrupt that process's log.

The local preview preparation helper now has an opt-in `--private-log` mode.
It compiles and signs `macos_preview_launcher.c` as the bundle executable.
Unless a caller provides `--logLocation`, the launcher makes a new private
`/tmp/vcmi-toroid-preview.*` directory and passes it to `vcmiclient`. Explicit
log-location arguments and other arguments are forwarded. Temporary logs
remain available until cleaned up by the user or operating system. This is a
local preview helper; it does not change the engine's default logging or
isolate server logs, saves, settings or multiplayer ports.

Two launcher tests passed on macOS arm64 using a recording client. They
checked distinct mode-0700 directories for two launches, argument forwarding
and both explicit `--logLocation` forms. The helper rejected an already
existing destination without replacing it. The separately prepared
`runtime/VCMI Toroid Preview f0f7f4a Finder.app` on TB5 passed deep strict
signature verification; the original `f0f7f4a` bundle was not replaced.

Finder's **Open Finder item** action started the new bundle without command
arguments. Computer Use observed version `1.8.0.f0f7f4a` and its main menu;
the process command line showed a private log directory, and `lsof` showed
only the pre-existing baseline client holding the default client log. The
new client exited through its in-game confirmation. Its client log is at
`runtime/gui-finder-f0f7f4a-2026-11-15/client.log` on the TB5 build root.
No new map was played in this run. The preceding
[Crossroads playthrough](topology-preview-f0f7f4a-2026-11-15.md) used the
same source revision in the earlier bundle. Finder's actual double-click
gesture, macOS access retention after reboot and visual smoothness remain
unverified. This is not a notarized or portable release.

## Current build: Finder double-click and gameplay

On macOS arm64, Finder double-clicked the separate signed
`runtime/VCMI Toroid Preview 9f20a0b Finder.app` without launch arguments.
The resulting process used its private temporary log and its window showed
`1.8.0.9f20a0b`. After the intro was dismissed, ordinary keyboard commands
opened the scenario list, selected `Toroid Crossroads`, and started a game.
The installed VMAP SHA-256 was
`dc22f9087595505366d7e6c6ff585df4e9b265436a18a2c7d6d539493848735f`,
matching the versioned map. The map started with the human player in control.
Two northwest key presses collected 1,000 gold and crossed both seams: the
server logged raw hero anchors `(1,0,0)` to `(36,35,0)` and 1,278 remaining
movement points. A static view after the crossing showed the road junction.
The client exited through the in-game confirmation and the local server closed
correctly. No manual save was made; a new scenario save directory was created.

The copied client log is
`runtime/gui-finder-9f20a0b-2026-11-15/client.log` under the TB5 build root,
SHA-256 `26efb49d3eea90825600e7d4a71b7d5dcfb8719ad09278ce0367409ad63fc121`.
This verifies one local startup and game path, not startup after reboot, a
portable release, or continuous animation and scrolling. The static views
were observed but not retained. Computer Use mouse and text-injection attempts
did not focus the map filter in this run; keyboard list navigation worked.
This does not establish whether a physical mouse or keyboard is affected.

While selecting an app by its shared display name, Computer Use additionally
started the old `f0f7f4a` bundle without its private-log launcher. It was
closed immediately, but its startup truncated the common client log also held
open by a pre-existing baseline process. The log now starts with the old
preview launch; no copy of its immediately preceding contents was found in
the log directory. The pre-existing baseline process was not stopped. The
current Finder preview's separate log was not affected.

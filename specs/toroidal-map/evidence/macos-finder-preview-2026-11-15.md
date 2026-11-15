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

# GUI-thread dialog warning

`CPlayerInterface::waitWhileDialog` previously logged a deadlock warning
whenever called from the GUI thread, including when no dialog was active.
Ordinary end-turn, load and exit confirmations reached this path in the
[earlier preview](topology-preview-eeb3540-2026-11-15.md). The GUI-thread
branch still returns without waiting, but now emits the warning only when
`showingDialog` is busy. Calls on other threads and dialog sequencing are
unchanged.

A macOS arm64 client built from `96315dd` plus this change was packaged in
`runtime/VCMI Toroid Dialog Check 96315dd.app` on TB5, signed using the
authorized Apple Development identity and verified with `codesign --deep
--strict`. The client reports the library's earlier `1.8.0.675f11b` version
stamp. Its signed client SHA-256 is
`2b18322840aa08e80c56b0e0070ec9161000221a959098aec3a5b6a00881a1cb`.

The directly launched client opened `Toroid Seam Demo`. Clicking end turn
displayed the normal unused-movement confirmation without the warning. While
that dialog remained open, Command-Q displayed an exit confirmation and the
warning appeared once. Cancelling exit returned to the original confirmation;
a diagnostic northwest shortcut left the hero at `(0,0,0)` with unchanged
movement points. Accepting the original dialog let Nullkiller2 finish its
turn and returned control to red. A later ordinary exit confirmation emitted
no additional instance of the warning. Client and local server closed
normally. A local autosave was created at the turn boundary.

This is one nested-dialog sequence, not a proof that the busy flag, queued
dialogs or asynchronous server questions are correctly ordered in every
case. The warning remains for the busy GUI-thread case. The client and test
targets built; 70 related AI and server dialog/activity tests passed. They do
not directly exercise this client warning. The build, focused-test log and
player-filtered GUI trace are under `runtime/dialog-idle-build-2026-11-15.log`,
`runtime/dialog-idle-targeted-2026-11-15.log`, and
`runtime/gui-dialog-idle-2026-11-15/` in the TB5 build root. Screenshots were
observed but not retained.

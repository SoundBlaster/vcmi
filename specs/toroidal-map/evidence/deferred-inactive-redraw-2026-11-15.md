# Deferred redraw after battle

The staged `Earth Seam Road QA` map was started on macOS arm64 with the
`109a663` client, an isolated HotA profile, `--testmap`, spectator mode and
AI players. The client terminated with `SIGSEGV` during the first day after
a battle. The crash report identifies the GUI thread in
`BattleStacksController::getHoveredStacksUnitIds`, called from
`StackQueue::StackBox::showAll` via `WindowHandler::processPendingRedraws`.
The faulting instruction loaded from offset `0xf8` with a null controller
pointer. The preceding client log recorded `BattleResultsApplied`,
`BattleEnded` and an object fade-out. The game was not using a fixed seed,
and this observation does not establish that toroidal movement caused it.

`CIntObject::redraw` draws only active widgets. The deferred path previously
checked activity when enqueuing but drew the widget unconditionally on a
later frame. Windows popped between those moments remain retained in
`WindowHandler::disposed` until a frame finishes, even if their battle
controller has gone away. Commit `ecc117d` checks `isActive()` again when
processing a deferred redraw, matching the synchronous path. This is a
targeted fix for inactive widgets; it is not a general proof of widget
lifetime safety or of the exact event ordering in the crash.

The client and test executable rebuilt on TB5 from `e1f2bce` plus the fix,
with existing duplicate-library and missing Conan search-path warnings. A
repeat on the same staged map used seed 17 and spectator mode; the server
processed `BattleResultsApplied`, `BattleEnded` and a subsequent object
fade-out without the prior crash. The test was stopped after 100 seconds;
it did not finish a game or prove normal shutdown. Since the original run had
no recorded seed, the two runs are not identical. The original and repeat
stdout logs are local under `runtime/earth-seam-road-qa-profile/` on TB5 as
`spectate.log` and `spectate-after-fix-long.log`; the macOS crash report is
`vcmiclient-2026-11-15-210457.ips` in the user's DiagnosticReports folder.
The screenshot attempt did not capture the seam, so visual acceptance of the
road remains open.

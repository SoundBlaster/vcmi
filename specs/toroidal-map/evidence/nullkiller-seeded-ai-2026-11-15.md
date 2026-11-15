# Seeded AI comparison

`--testseed` selects a positive 32-bit seed for a local, in-process `--testmap`
session without changing persistent settings. It is rejected without a test
map, with `--donotstartserver`, or with an out-of-range value. Separate server
processes are explicitly unsupported. The server now logs the requested seed
when one exists and labels the previously logged RNG output as the first random
value. Therefore, the `440640067` reported as a seed in the earlier
[AI session](topology-ai-game-06ea341-2026-11-15.md) is an output, not an
established initial seed. Fixing the game RNG seed does not promise identical
thread scheduling or every AI decision.

On macOS arm64, the client and server were built from `7cdce47` plus this
change. With seed `123`, two headless runs of the clean 36×36 toroidal demo
both logged first value `414464415` and completed on day 2. A bounded control
with the same seed logged the same first value and completed on day 5. The
control archive differs from the original only in the topology field and map
format major version; all other archive members are byte-identical. Its
SHA-256 is `68274660ab90143c2f9ec0ecdd242081116c3d1240f377c4c944338ae6fb0170`.

On the toroidal map, seeds 1, 2, 3, 5 and 6 completed by days 34, 9, 12, 15
and 13 respectively. The seed-4 run was manually interrupted on day 37; a
repeat was interrupted on day 56. Both repeatedly reserved town defenders and
reported no worthwhile task. The bounded seed-4 control was interrupted on
day 68 after also repeatedly reserving defenders and reporting no worthwhile
task. These are observations of a sparse synthetic fixture, not proof of a
permanent deadlock or of the same root cause in all runs. In particular,
seed 1 eventually ended after a number of no-task turns. The bounded case
shows that the observed pattern is not exclusive to wrapping. The seed-4
case remains a reproducible investigation target, not a resolved AI defect.

CLI checks rejected zero, an overflowing seed, a missing test map and a
remote-server request. The focused network and movement suite passed 17
tests with two optional external-server tests skipped. The user's settings
file retained SHA-256
`a04043a03263ce8569341fb84038b5c76d481f5b3f1d615b50b23ee87263a770`.
Logs and the XML report are in `runtime/ai-seed-2026-11-15/` on the TB5
build root. The build emitted existing duplicate-library and missing Conan
search-path warnings. No graphical behavior, remote game or AI strategy fix
was verified here.

## Seed 4 on the current build

On macOS arm64, the client built from `9f20a0b` (the later `11e9d55`
revision changed documentation only) ran two headless, in-process games with
seed 4 and a private log directory per game. Copies of the earlier toroidal
fixture and its bounded control used distinct scenario names to isolate saves.
The copies' `objects.json` and `surface_terrain.json` were byte-identical;
their headers differed in the toroidal flag and format major version, and
their Russian text differed in the scenario name. Both logs reported requested
seed 4 and first random value `1916493703`.

The toroidal run exited normally on day 69 after blue's victory and red's
elimination. It logged 92 full-scan no-task outcomes and 135 defender
reservations across both players. The bounded control had not finished at the
75-day limit and was interrupted; polling observed it at day 79. It logged
97 full-scan no-task outcomes and 139 defender reservations. Both sessions
continued to process turns, and the toroidal run therefore demonstrates that
this particular run was not a permanent game deadlock. The bounded control
was stopped before its outcome, so it provides no comparable completion day.
These counts are log-line counts, not counts of distinct heroes or proven
strategic errors. A fixed game RNG seed does not control scheduling or all AI
decisions; the two games should not be treated as identical trajectories.

The full logs and run summaries are under
`runtime/seed4-current-11e9d55-2026-11-15/` on TB5. Client log SHA-256 values
are `44a6fa18c1a0a21b291a7951bd359522f626fe8a386bc1f202e39317d4a9f758`
for the toroidal run and
`b30ebd2a708377809e8ee5766d12a1105e0900b652a0da1b7ced7c41e2e555e6`
for the bounded control. The stop condition is polled, so the recorded limit
may overshoot, and the summary's final-log day is more accurate than its
last polled day. This does not prove balanced strategy, general AI liveness,
remote play or any graphical behavior.

## Crossroads seed 4 on 8b680f3

The current macOS arm64 build ran the versioned 36×36 Toroid Crossroads
scenario with two AI players, seed 4, a local in-process server and a private
log directory. A hero crossed the horizontal seam successfully. The game had
not ended by day 1316 when the 120-second limit interrupted it. The log also
repeatedly reported no worthwhile task. This is one run, without a bounded
control; it does not establish a permanent deadlock or a topology-specific
cause. It does not establish graphical or remote-network behavior.

The run summary and logs are under
`runtime/crossroads-ai-8b680f3-2026-11-15/` on the TB5 build root. The user
settings file retained SHA-256
`a04043a03263ce8569341fb84038b5c76d481f5b3f1d615b50b23ee87263a770`.

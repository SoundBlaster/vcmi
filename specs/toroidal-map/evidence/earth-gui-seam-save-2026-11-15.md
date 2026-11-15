# Earth draft: graphical seam crossing and save/load

On macOS arm64, the SDL3/Metal client built from `32936c5` with the
diagnostic shortcuts from `17c6a69` started a local test copy of the Earth
draft through the normal scenario list. The red player was human. The
252×576 map had toroidal mode enabled and the client used a separate profile
and an explicitly enabled private diagnostic socket.

The fixture was derived from `earth-reserved-guard-diagnostic.vmap` (SHA-256
`58e7b7e6bf94c117d350028757c9cf1c7160bf3107fe435cf003306d3dbd74a6`). Its
only changes were renaming the scenario to `Earth Seam QA` and moving red's
starting hero, `randomHero_7305`, from object anchor `(165,353,0)` to
`(1,425,0)`. The resulting initial visitable tile was `(0,425,0)`. This
places the hero next to a horizontal seam previously exercised by the
in-process Earth checks; it is not an unmodified playthrough of the demo.
The derived fixture has SHA-256
`a0ddb467b05140c2c63d9bc19e2d15cd7df69ef9ed7cf28580baf028ab935863`.

The diagnostic channel sent directional and quick-save/load shortcuts:

| Step | Selected hero tile | Movement points |
| --- | --- | --- |
| Start | `(0,425,0)` | 1,560 |
| West across seam, then quick-save | `(251,425,0)` | 1,410 |
| East across seam | `(0,425,0)` | 1,260 |
| Quick-load and confirm | `(251,425,0)` | 1,410 |
| East across seam after load | `(0,425,0)` | 1,260 |

The observation stream showed the same hero ID, mana, resources, map size,
topology and viewport `visible_runs` at the saved and restored states. This
compares only the recorded viewport visibility, not the complete fog map.
The client and its local server shut down normally, with client exit code 0.

The fixture, isolated quick save and logs are local to TB5 under
`runtime/earth-seam-gui-profile/`; no source map or game assets are committed.
At the time of this run, `observation.jsonl` had SHA-256
`2786c7ab01faf9916bcbf6359d41b04142c877edef78222345e3fc0b089c8d7d`,
the client log had SHA-256
`13f3c4dde72ae75f3ee00ee71011339d6f1b63742b8d57dcc713d1cbc203bb49`,
and the quick save had SHA-256
`204ec89a844253d5c3fe3288fe185f83534cf845d79e5f726c7a77df94116e2b`.

This checks one unobstructed horizontal land seam and persistence in an
actual human-controlled graphical session. It does not establish the quality
of seam animation, a vertical or corner graphical crossing on Earth, full
fog equivalence, remote multiplayer, or general map playability.

# Earth demo: graphical quick save and reload

On macOS arm64, the SDL3/Metal client built from `32936c5` with the diagnostic
shortcut additions committed as `17c6a69` started the local Earth draft from
the ordinary scenario list. It used the isolated profile described in the
[profile record](earth-isolated-profile-2026-11-15.md); the red player was
human. The diagnostic socket was explicitly enabled in a private directory.

The test sent an eastward movement command, quick-saved, moved east again,
quick-loaded and accepted the confirmation, then moved east once more:

| Step | Selected hero tile | Movement points |
| --- | --- | --- |
| Start | `(164,353,0)` | 1,560 |
| Before save | `(165,353,0)` | 1,510 |
| After second step | `(166,353,0)` | 1,460 |
| After load | `(165,353,0)` | 1,510 |
| After new step | `(166,353,0)` | 1,460 |

The observation stream reported toroidal mode and 252×576 dimensions at
each recorded state; the selected hero ID and displayed resources were also
the same before save and after load. The server log recorded a successful
save to `Quicksave.vsgm1` and a subsequent load. The client then exited with
code 0 after the test requested and confirmed exit. The quick save remains in
the isolated profile, not the ordinary user profile.

The local run directory is `earth-save-gui-tngovjc6/` under the TB5 Earth
research runtime. `observation.jsonl` has SHA-256
`00d19336d7b98b8cb1b494a1aff1346a5d65efd6a419130f40ab91eab2b003ce`, and
`VCMI_Client_log.txt` has SHA-256
`edadebd11b685f7e92a90c3d66085bad1ec765a822abddf5d030e9d72b39b6bd`.
The saved file's SHA-256 was
`49622c71ef8771935edeae4300566bda59d86c02bb5822ac9cd7d4782616eaf5`
at the time of the test; later play may overwrite it.

This run did not cross a seam, compare complete fog or state, test remote
peers, or establish animation continuity. Separate in-process checks cover
selected seam crossings and fog after save/load; see the
[Earth proposal](../earth-map-demo-proposal-2026-11-15.md).

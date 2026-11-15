# Earth draft: graphical vertical seam and save/load

On macOS arm64, the SDL3/Metal client built from `32936c5` with diagnostic
shortcuts from `17c6a69` started `Earth Vertical QA` from the scenario list
with a human red player and an isolated profile. The 252×576 map was toroidal.
The test copy was derived from the Earth draft SHA-256
`58e7b7e6bf94c117d350028757c9cf1c7160bf3107fe435cf003306d3dbd74a6`;
only the scenario name and red starting hero anchor were changed. The anchor
was moved from `(165,353,0)` to `(1,0,0)`, giving visitable tile `(0,0,0)`.
The test fixture SHA-256 is
`6e61430be10bc2bc896fae3d1601424e3a029b9504e73e02c13e40e0a05a8380`.

| Operation | Hero tile | Movement points |
| --- | --- | --- |
| Start | `(0,0,0)` | 1,500 |
| North across seam; quick-save | `(0,575,0)` | 1,400 |
| South across seam | `(0,0,0)` | 1,300 |
| Quick-load and confirm | `(0,575,0)` | 1,400 |
| South after load | `(0,0,0)` | 1,300 |

At save and load, the observed hero ID, mana, resources, topology, dimensions
and viewport `visible_runs` matched. This is not a comparison of the full
fog map. A subsequent northwest shortcut was accepted by the diagnostic
channel, but no movement was observed within eight seconds. The reason was
not established; this is not evidence of a successful corner crossing or
of a topology defect. The client and local server exited normally; client
exit code was 0.

Local evidence is under TB5 `runtime/earth-vertical-gui-profile/`.
Observation SHA-256:
`5f7949dd572c535e998fa46599104d050f240a441000199de524843d4eec40ef`.
Client log SHA-256:
`f2a2e4b9b8d12c835b5ed20d5fce4c3ca77c9a9899e4752e97620a744649a385`.
Quick-save SHA-256:
`a0ea26fd67554926a2c9c18815f6635ebcf28e5778e8caac142ed56018bab7a1`.
The fixture and game assets remain local. Animation continuity, all corner
crossings, full fog, multiplayer and overall playability remain unverified.

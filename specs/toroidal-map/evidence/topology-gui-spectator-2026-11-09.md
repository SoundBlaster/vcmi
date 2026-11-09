# Toroidal viewport: spectator observation

On 2026-11-09, macOS arm64, the SDL3 client was run with copied Complete
assets and `--testmap Maps/toroidal-demo.vmap`. The build used source based on
`da734d1`, with the loader and language changes later committed as `38ffb84`
and `a8d5132`; its test-only changes do not affect this client observation.
An experimental `.app` on TB5 used the built Mach-O with links to its build
resources and server. The map was synthesized from the demo and 98 static
objects were relocated to meet the current footprint restriction; see the
[integration record](topology-demo-integration-2026-11-09.md).

The client log identifies the session as a spectator. In this mode the
viewport intentionally reveals the map, so this run cannot establish player
fog behavior. The minimap remained mostly black, which is not by itself proof
of a visibility leak.

Using `Ctrl+Left` and `Ctrl+Up` moved the viewport across the west and north
edges. The minimap radar was split at the boundaries. Seven 2306×1448
screenshots are retained outside the repository under the TB5 build root in
`runtime/evidence/gui-toroidal/`:

```text
45a19c17f84d5b1a883c89ab6d359ca00d0a0e3c04d6f4487c4a29db797ba69e  before-west.png
ef2e7df445d5bea5734ba18f4b1ceab71cf92f5f63756dc58950fd7eb31bedbb  after-west.png
f0babf3692ecbe9c113c5fa105444edebaab098c35210d46f1a1a09cc1adc2c8  frame-01.png
c44c64d625e3465f1a187ac2042344e3e542aec6eec82b19555d6fd8966ff1d6  frame-02.png
27c2734b06dc8ac43abb3a4f0a456fcab41adf587cc9a26deb19608d5ea1cf59  frame-03.png
d2a254d1208f75dc23afa1095cea366851550edb8cfc6b4c80c996b54120e96a  frame-04.png
4cd857ea83b2eb74b733b963a603b18722d97d20ceb4db965959d18fee5ee516  after-north-west.png
```

The four numbered frames are discrete positions after 350 ms input bursts,
not a continuous recording. They do not prove frame-by-frame continuity or
hero and boat animations. The original demo art was not authored to connect
its opposing edges, and visible terrain discontinuities remain at the seam.
Real-player fog, input at each seam, appropriate seam art, other directions,
wide zoom, and acceptance remain unverified.

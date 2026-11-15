# Preview with minimap icon changes

The macOS arm64 client and server were built from clean revision `d61b3bd`
under the checkout-specific TB5 build root. A separate local development
preview was assembled at `runtime/VCMI Toroid Preview d61b3bd.app` from the
prior preview, with the new client, server and engine library. The app keeps
the previous preview's bundle identifier and uses the authorized local Apple
Development identity. `codesign --verify --deep --strict` passed. Some other
dependencies still resolve through the TB5 build toolchain, so this is not a
portable or notarized distribution.

The signed client was invoked directly with a private diagnostic socket and
log. It reported `1.8.0.d61b3bd`, used the Metal renderer, and opened the
36×36 `Toroid Seam Demo`. Player observation reported a toroidal map, hero
at `(0,0,0)`, 1,560 movement points and 20,000 gold. One northwest shortcut
collected 1,000 gold and left the hero in place with 1,419 movement points;
the next moved the hero to `(35,35,0)` with 1,278 points. The GUI was
observed before and after the transition. The client exited with code 0,
logged a clean server shutdown, and port 3030 was no longer listening. No
manual save was made; scenario directories may have been created.

The client log and observation stream are in
`runtime/gui-preview-d61b3bd-2026-11-15/` on the TB5 build root. The
observation stream has SHA-256
`e8e9f4865eb7f22fadddc26131f324cc19ec97514f43e90465877d2e55b563ec`.
The screenshot was observed but not retained. The hero's minimap icon was too
small in the full-window view to establish that its clipped fragments appear
on the opposite edges. See the [software-canvas and geometry
checks](minimap-wrapped-hero-icons-2026-11-15.md). This run also does not
establish continuous animation, a human-accepted scrolling result, remote
network play, or an AI turn. Launch Services and interactive Terminal access
to TB5 were not checked for this bundle.

Signed binary SHA-256 values, in client, server, library order:

```text
21311765aecc868bf58a29344237610b58db19d32b746b53f81e89a2615bc6cd
c580343fae38edb1e3c865687f98c8f254e7f48ac42a0f0fad47de7c76f9dab5
4dc74f6c141818d10d32b732c475890aa1643a889bfe7884538aed3837dc7f13
```

# Prepared preview bundle and pending launch

On macOS arm64, client, server, editor and Nullkiller2 targets built on TB5
from clean revision `961e235`. A separate local `VCMI Toroid Preview 961e235`
application was copied from the previous test bundle; its client, server and
`libvcmi` were replaced with the new binaries, linked to the bundled libraries
and signed with the authorized development identity. The editor target built,
but the editor executable in this copied application was not replaced or
validated. The app's command socket and observation log were configured
separately from the already-running test client. `codesign --verify --deep
--strict` passed.

After signing, SHA-256 values were:

```text
6fefac8079f4daf347fc1a1fea02d2cd96d1059538e3bc4cc2774eb0cb7bfad1  vcmiclient
b1b95ab141392ffbfb10a4edf0b33601628fc2bf3da2331815bd4cbd331b1574  vcmiserver
59aa0556c0a62c8a5e0100c65d63714792f895d58742f09fdf6515ce7bfd9d68  libvcmi.dylib
```

The launch produced a live client process but no observed game window. A
two-second macOS process sample showed its main thread waiting in
`CBasicLogConfigurator::configureDefault` while opening a log file. The
configured log directory is on TB5 and remained empty. A system access
prompt was not visible to the agent, so its presence or absence and the cause
of the wait are not established. The previous `a4b710a` client was left
running separately. This record establishes build and signing, not successful
startup, map loading or gameplay on `961e235`.

After the user granted disk access, that same process continued startup; the
client window identified itself as `1.8.0.961e235`. The diagnostic channel
selected and loaded `Toroid Open Water Corner` `Autosave-111`. Computer Use
showed its adventure map, and the observation stream reported a 36×36
toroidal map with the selected hero at `(31,30,0)` and zero movement points.
This establishes startup and save loading on that build, not a new crossing,
continuous animation or the later minimap pixel-alignment change.

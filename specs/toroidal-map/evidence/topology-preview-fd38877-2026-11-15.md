# Local preview and Launch Services investigation

The macOS arm64 client and server were built from clean revision `fd38877`.
A separate local app was assembled from an earlier development preview with
the new client, server and engine library, using the previously authorized
Apple Development identity. The initial app still had absolute Conan build
RPATHs. Launch Services created its process, but after approximately 30 seconds
`sample` showed its main thread in `dyld` waiting in `open`; no client log or
window was observed. This identifies the wait location, not the kernel or
privacy decision behind it.

The executable and its bundled libraries were then prepared with their
non-system dependencies inside `Contents/Frameworks` and their absolute build
RPATHs removed. Signature verification passed. A second Launch Services run
loaded the libraries and reached the client, then waited in `fopen` while
opening the requested log on TB5. A third run with a private `/tmp` log reached
filesystem initialization and waited in `open` while scanning the existing
user save directory. Samples were taken of both waits. No system authorization
dialog was confirmed. All three test processes were intentionally terminated;
no other VCMI processes were touched. This does not establish that the app
opens successfully from Finder or identify the operating-system cause.

`specs/toroidal-map/tools/prepare_macos_preview.py` makes this library-path
transformation reproducible for a *new* local copy. It requires an existing
bundle containing the client, server and non-system libraries, checks those
dependencies, refuses to overwrite an output, removes non-system absolute
RPATHs, adds bundle-local paths, signs and verifies the copy. It does not build
the app, install original game data, configure privacy permissions, notarize
or create a portable release. Only the client, server and direct Frameworks
libraries are checked by the helper; other bundled programs are outside its
dependency audit.

The final helper created `runtime/VCMI Toroid Preview fd38877 local.app` on
the TB5 checkout-specific build root. Sixteen inspected Mach-O files had no
non-system absolute RPATHs; `codesign --verify --deep --strict` and a direct
`vcmiclient --version` invocation passed. An existing output was rejected
without replacement; a separate copy missing `libminizip.dylib` was rejected
before the destination was created. The source bundle remained signature-valid.
From a direct process invocation, the app opened the clean toroidal demo and displayed version
`1.8.0.fd38877`; its observation channel reported a 36×36 toroidal map.
Both players were AI in this test. The app exited with code 0 after the test
was closed, and its server reported a clean shutdown. The screenshot was
observed but not retained; this is not visual acceptance of scrolling or a
human-controlled game.

The demo VMAP retained SHA-256
`9a945a811529805ac53c28352027e01129446f434847497b0512d770600e1c0a`;
the user's `settings.json` remained at
`a04043a03263ce8569341fb84038b5c76d481f5b3f1d615b50b23ee87263a770`.
Process samples are under `runtime/launch-services-fd38877-2026-11-15/` and
`runtime/launch-services-bundled-fd38877-2026-11-15/`; client log and
observation trace are under `runtime/preview-fd38877-local-2026-11-15/`.
No manual game save was made; the game may create save directories itself.

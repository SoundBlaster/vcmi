# Current preview: Launch Services and Crossroads

On macOS arm64 the client, server and engine library were built from clean
revision `f0f7f4a`. A separate bundle,
`runtime/VCMI Toroid Preview f0f7f4a.app` under the TB5 build root, was
prepared with `prepare_macos_preview.py` from the previous local preview and
the new binaries. Its existing bundle identifier and authorized Apple
Development identity were retained. Deep strict signature verification
passed. It is a local development bundle, not a notarized distribution;
the helper does not audit every program in the bundle.

After the user granted TB5 access, Launch Services opened this bundle with a
private temporary log, diagnostic socket and player observation stream. No
new disk permission prompt was observed in this run. The window identified
version `1.8.0.f0f7f4a`, and the installed `Toroid Crossroads` map selected
through the GUI had SHA-256
`dc22f9087595505366d7e6c6ff585df4e9b265436a18a2c7d6d539493848735f`, matching
the versioned VMAP. The new local game reported a 36×36 toroidal map.

Two northwest commands collected 1,000 gold and moved the red hero from
`(0,0,0)` to `(35,35,0)`; observed movement fell from 1,560 to 1,278. In
one static Computer Use view after the crossing, the horizontal and vertical
dirt roads met continuously at the `(0,0)` junction across both seams. The
game exited through its in-game confirmation; no manual save was made. A
scenario save directory was created. Client log and observation stream are
under `runtime/gui-preview-f0f7f4a-2026-11-15/` on TB5; the screenshot was
observed but not retained.

This run checks one game start and corner transition, not the AI spell fix in
live play, a complete match, an actual Finder double-click, other game
installations or uninterrupted scrolling and animation. The user will assess
visual smoothness separately. Launch Services was supplied diagnostic
environment variables and a temporary log argument for this run.

SHA-256 of the packaged files after signing:

```text
3520b228decd6ebeeb0ef3a548ce238c6bc45faf039e8fb825c339a1e630897b  vcmiclient
38ce154519f59d0e024bf5269fa657de3955da7a44791919003474d070039f39  vcmiserver
94e74d13ef86303f3983a09ae199e0d5ed95c5354014b755a411866489fdd61d  libvcmi.dylib
```

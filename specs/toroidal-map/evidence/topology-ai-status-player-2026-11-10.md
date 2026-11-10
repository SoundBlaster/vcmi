# Player-facing AI status

The scenario information card now prefixes the map description with an
experimental wraparound and unverified AI notice when the selected map header
is toroidal. Bounded maps keep their original description. The notice applies
to all profiles because none has yet been accepted as fully supported; it
does not claim that partially tested Nullkiller2 never works. English and
Russian strings were added, with the base English string available as the
existing translation fallback.

The macOS arm64 client target built successfully on TB5 and both translation
files were parsed and checked for the new key. The current client did not
reach a game window during the separate launch diagnostics, so placement,
readability, other languages and actual profile-selection flow have not been
visually verified. The underlying AI behavior remains only partially tested.

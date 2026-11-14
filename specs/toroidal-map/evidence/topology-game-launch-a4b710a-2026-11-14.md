# Current-client launch and diagonal boat crossing

On macOS arm64, the clean `a4b710a` client, server and shared library were
built on TB5, copied into the existing `VCMI Toroid Commands` test bundle,
rewritten to use its bundled libraries, signed with the authorized Apple
Development identity and verified with `codesign --verify --deep --strict`.
The client window identified itself as `1.8.0.a4b710a`. The bundled client,
server and library SHA-256 hashes after rewriting and signing were,
respectively, `46b95bf120f6fb07eec40fd567daf68de48a9d7399f044d05ea6533ca053301b`,
`aa9092d0d1476cfed079c25e131cebe570b97619966ad822ff934cd07a484115`, and
`52ff2bffab1eacb920d278f3dbf2058f015456d2d2f2442e78f57533320c445b`.

The graphical client loaded the existing `Toroid Open Water Corner` save and
displayed its hero on a boat. The observation channel reported a 36×36
toroidal map. The game advanced to day two, then diagnostic directional
commands moved the boat `(31,30) → (32,31) → (33,32) → (34,33) → (35,34) →
(35,35) → (0,0) → (35,35)`. Both corner steps spent 141 movement points;
the last observed position had 554. Computer Use showed the final boat and
minimap. The local observation records 0–28 are retained on TB5 as
`runtime/observation-boat-a4b710a-2026-11-14.jsonl`, SHA-256
`cf1799715838986cfaef09124b07443246d73cdf5c1b4ba837edbd7eddd9271e`.

This is a startup, save-load, day transition and ordinary-boat crossing check,
not a visual demonstration of the aviation recovery change or continuous
frame pacing. No separate server process was observed; this check does not
establish a remote server deployment or multiplayer session. The build emitted
duplicate-library and missing-search-directory linker warnings.

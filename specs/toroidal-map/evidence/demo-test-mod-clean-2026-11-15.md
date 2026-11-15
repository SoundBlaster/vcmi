# Demo maps without test-only entities

The laboratory maps produced by `vcmitest` inherited the test registry's
`vcmi-test` heroes and spells when the synthetic H3M was loaded and saved as
VMAP. In an ordinary game without that test mod, loading the previous prototype
logged unresolved test-spell errors. The synthetic demo generators now remove
only `vcmi-test` heroes and spells from map allowlists and town spell lists
before serialization. Five generated fixtures check their header and object
JSON for any remaining `vcmi-test:` reference. This is specific to synthetic
test fixtures; it does not strip entities from user maps or general map saves.
Other fixture generators and previously generated VMAP files are not migrated.

Five focused fixture tests passed. With the prototype VMAP, save and Complete
H3M fixtures supplied, the full local macOS arm64 suite passed 1,663 tests;
four were disabled. The logs are `runtime/demo-clean-map-generation.log` and
`runtime/full-suite-clean-demo-2026-11-15.log` under the TB5 build root.

Two new, separately named maps were generated under
`runtime/complete-data/Maps/` on TB5:

| Scenario | File | SHA-256 |
| --- | --- | --- |
| Toroid Seam Demo | `toroid-demo-clean-2026-11-15.vmap` | `9a945a811529805ac53c28352027e01129446f434847497b0512d770600e1c0a` |
| Toroid Roads Demo | `toroid-road-axes-clean-2026-11-15.vmap` | `b451f983719e345fed98cc13f6ac5bd5472a6b3b943a4bfd60c68c291213bc74` |

The [local preview client](topology-preview-eeb3540-2026-11-15.md) opened
`Toroid Seam Demo` with the ordinary game content. The server loaded the map
and started Nullkiller2 for blue. The diagnostic observations recorded red
collecting 1,000 gold and crossing from `(0,0,0)` to `(35,35,0)` with 1,278
movement points. The run's log contains none of the six `vcmi-test` identifiers
found in the earlier prototype. The game was closed without a manual save.
This check did not play an AI turn on the new map or exercise all objects;
the newly generated road fixture was not visually inspected in this run.

The log and observation trace are in `runtime/gui-clean-demo-2026-11-15/`
on TB5. The list of other installed maps produced separate H3M validation
warnings and unsupported HotA map-format errors. The end-game confirmation
also logged the GUI-thread warning described in the preview record.

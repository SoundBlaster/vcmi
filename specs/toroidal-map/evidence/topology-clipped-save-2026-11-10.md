# Save and restore of fully clipped object state

The in-memory fix at `d6e25b0` lost the distinction between a shown fully
clipped object and a hidden object when reloading a game save. The new
branch-local `TOROIDAL_UNINDEXED_OBJECTS` serialization version stores IDs of
shown unindexed objects in map-object order. On load, IDs are checked for a
present matching object, duplicates, a real blocking or visitable cell, and
absence of indexed occupancy. Invalid entries cause loading to fail. Old
versions do not contain this field and cannot recover the distinction.

The server chooses the previous untagged version for an ordinary bounded map.
It chooses the new tagged version for a toroidal map or for a bounded map
with at least one shown fully clipped object. This special bounded save remains
bounded after loading, but readers without the branch-local version must reject
it rather than silently ignore state. The marker is not an upstream allocation;
no old binary was run to verify its rejection. Older bounded or toroidal saves
remain readable by the new reader subject to the existing format and mod
checks. An older save has no information from which to reconstruct the new
state. The previous tagged toroidal peer version remains negotiable; the new
field is version-gated in the shared serialization stream.

`ToroidalDemoGameTest.ServerSaveUsesTheTopologySpecificSerializationVersion`
uses the real server save method for three cases: ordinary bounded, toroidal,
and bounded with one shown and one hidden fully clipped resource. It checks
file versions, reads the header and game state, switches the restored map to
toroidal topology, and finds only the shown resource at its wrapped position;
the server handler also loads the file. The older untagged prototype format,
legacy bounded and current toroidal save tests, prior tagged-peer negotiation,
and TCP negotiation passed. A real older GUI prototype save with SHA-256
`ad2a9df20f1995f62ffd215d62a6e8fa8cb4856443aa9302144f19d864d3ca73`
was decoded without rewriting it; checksum before and after was unchanged.
This direct decode does not establish mod compatibility or resumed GUI play.

On macOS arm64, source `d6e25b0` plus this change built `vcmitest` on TB5;
26 related tests passed before the last test adjustment, and the final full
run passed 1,457 tests with two optional-input skips and four disabled tests.
It included the real old prototype save. Log:
`/Volumes/TB5/Developer/Builds/GitHub/VCMI-8dd012bf29c2d973/full-suite-clipped-save-version-2026-11-10.log`.
Linker warnings about duplicate libraries and missing Conan search paths
remain. The changed source has not been exercised through a GUI save or a
remote multiplayer host.

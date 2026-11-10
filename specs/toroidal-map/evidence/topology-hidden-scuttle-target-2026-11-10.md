# Hidden Scuttle Boat targets on a toroidal map

`SummonBoatTopologyTest.scuttleBoatPlayerTargetCheckDoesNotExposeHiddenBoatAcrossSeam`
places a boat across the horizontal seam, with a hero holding Scuttle Boat.
With the destination revealed, the player callback accepts the target. After
only that destination tile is hidden, the same callback must reject it;
the unrestricted game-state callback still accepts the physical target. On
reveal, the player callback accepts it again.

Before the change the player callback accepted the hidden target. The
`removeObject` ranged effect now checks player-visible tile access for a
toroidal map when the effect does not ignore fog. The unrestricted callback
still sees the authoritative map; bounded behavior and the configuration
for effects that explicitly ignore fog are not changed by this condition.
This is a target-preview check, not a claim of complete fog confidentiality
or of network secrecy. It also does not test a real client cursor or cast.

On macOS arm64 the client library and `vcmitest` built in the TB5 CMake
directory. The focused pre-fix test failed as described. After the fix, 24
related tests passed; after the final assertion was added, the full suite
passed 1,452 tests, skipped three optional external-input cases, and reported
five disabled tests. No TCP test was excluded. Logs under the TB5 build root:
`scuttle-hidden-before-test.log`, `scuttle-hidden-final-build.log`,
`scuttle-hidden-after-tests.log`, and `full-suite-scuttle-hidden.log`.

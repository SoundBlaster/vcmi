# Server movement source layer

On a toroidal map, possession of Flight previously made the server skip the
source-object exit rule even when the hero was standing on land. The extended
`HeroMovementTest.PathfinderHonorsSourceObjectExitDirectionAcrossSeam` first
reproduced a forbidden LAND step that the server accepted while the pathfinder
rejected it. It now checks both LAND and AIR requests from the restricted
ground tile are rejected without moving the hero. The hero then enters that
tile in AIR from a permitted neighboring tile, crosses the seam and lands.
The test also checks that a rejected move does not change the source layer,
and that after landing the hero again obeys restricted ground exits.

`CGameHandler` tracks the last server-confirmed layer and visitable position
for each moved hero on toroidal maps. The source direction check applies to
LAND and SAIL, not AIR or WATER. A changed position cannot reuse a recorded
layer; server `changeObjPos` and `removeObject` clear the record. The code
continues to apply the earlier permissive fallback when the layer is not
known, including after reading a prior server save. It does not infer a past
layer from a movement capability or trust a client-provided source layer.

The new branch-local `TOROIDAL_MOVEMENT_SOURCE_LAYER` serialization version
stores this server state and whether older-save defaults remain unknown.
New ordinary bounded saves still use the previous bounded version. Tagged
toroidal or special bounded saves use the new version; older branch saves
load without the field. This is an experimental version, not an upstream
allocation. Invalid serialized hero identifiers or layer values are rejected
by the new reader; corrupted-layer handling has not been independently tested.

The source-exit fixture serializes the server handler while airborne, restores
it in memory and continues after a rejected move. It also reads an older
handler layout without the new field. The disk-save fixture
`ToroidalDemoGameTest.ServerSaveUsesTheTopologySpecificSerializationVersion`
writes a real save with a hero in AIR over a restricted resource, loads its
game and server chunks and lands across the seam. It additionally invokes the
normal server load path and checks version selection for bounded and toroidal
cases. These are controlled test fixtures, not a graphical save/resume.

On macOS arm64, `vcmitest`, `vcmiserver` and `vcmiclient` built from
`43d8854` plus this change. The complete suite passed 1,468 tests from 226
suites, with no skips and four disabled. The optional inputs were the older
untagged prototype save and VMAP and the Complete `Dead and Buried.h3m`;
their SHA-256 values remained unchanged after the test. The log is
`full-suite-persisted-source-layer-final-2026-11-10.log` in the TB5 runtime
directory. Build linker warnings about duplicate libraries and unavailable
Conan search paths remain.

The layer is server-side; the ordinary client pathfinder's initial-node logic
still derives a starting layer from boat ownership rather than this server
state. Replanning after an interrupted flight or water walk, synchronization
with graphical clients, all nonstandard relocations and bonus expiry,
multiplayer continuity, and full legacy save gameplay remain unverified.
The raw mismatched network-codec layout regression remains disabled. Older
saves cannot recover information they never stored.

The later [replicated-layer follow-up](topology-replicated-movement-layer-2026-11-10.md)
changes the initial-layer behavior described in the previous paragraph and
adds scoped local, TCP and save checks; its remaining limitations are listed
there.

# Visibility of a fully wrapped object

An unrestricted `CGameInfoCallback` previously decided whether an object was
on the map by checking its raw bounding cells against the rectangular bounds.
An object whose entire footprint lay outside the rectangle was therefore
reported invisible even when a toroidal map wrapped it onto a valid tile.

The unrestricted check now uses the map topology for each bounding cell;
invalid levels and bounded-map outside positions remain invalid. The
player-specific branch remains separate and still applies fog of war.
`HeroMovementTest.UnrestrictedCallbackSeesObjectFullyAcrossToroidalCorner`
failed before the change and passed afterward. It checks a one-cell object
across both seams on an underground level, bounded behavior, hidden and
revealed player visibility, and an invalid level. Three related object,
team-visibility and TCP movement tests also passed. The macOS arm64 client,
server, editor and test targets built with the existing linker warnings.
This is callback-level evidence; it does not verify a spectator GUI or prove
confidentiality of all network messages.

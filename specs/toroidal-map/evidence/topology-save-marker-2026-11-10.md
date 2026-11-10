# Experimental save marker evidence

The previous branch writer stored its ordinary serialization ordinal for all
maps. The new marker test initially failed: the on-disk value was 940 with no
high-word discriminator. The branch now tags newly written toroidal save
versions with `0x5452` in the high word, while the local reader decodes the
ordinary version before interpreting chunks. An unmodified reader using the
existing numeric newer-version check would reject this value for a full load;
an actual older binary has not been run against it.

The real `CGameHandler::save` selects the previous serialization version for a
bounded map and the tagged current version for a toroidal map. A test runs
the server save and load methods for both versions using a temporary
filesystem overlay on TB5, checks the raw file version and loads map header,
game state and server state. Separate tests cover old untagged prototype
toroidal state, malformed and future tagged versions, metadata-only access
and chunk integrity. The focused 17-test save suite passed on macOS arm64;
client and server targets built on TB5.
The complete local suite passed 1,412 tests with four disabled
(`full-suite-save-marker-final.log` on TB5); `specpm validate . --json`
reported no errors or warnings. This is not an upstream CI result.

The marker is experimental and not reserved upstream. Old untagged prototype
saves remain readable but cannot be distinguished reliably from an unrelated
future format using the same ordinal. VMAP major-version collisions and GUI
save/reload on the new build are also unverified. No existing save was
rewritten by this change.

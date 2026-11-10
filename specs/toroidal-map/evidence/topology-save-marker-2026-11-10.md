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
future format using the same ordinal. New VMAPs subsequently received a
separate branch-local marker; untagged version-4 prototypes remain ambiguous.
GUI save/reload on the new build is also unverified. No existing save was
rewritten by this change.

The optional `ToroidalDemoVmapTest.DecodesExternalUntaggedPrototypeSave` was
also run against the earlier GUI save `НОВАЯ ИГРА.vsgm1` in the isolated
TB5 laboratory data. Its raw version was 940 and its SHA-256 was
`ad2a9df20f1995f62ffd215d62a6e8fa8cb4856443aa9302144f19d864d3ca73`.
The test read it without writing it, fully decoded the saved header, game
state and server state, and confirmed toroidal topology and map dimensions.
The normal `CGameHandler::load` attempt first failed its mod-compatibility
check because the test preset and saved game differ; direct decoding does not
prove that the normal loader accepts the saved mod set or that play can resume.
Set `VCMI_TOROID_PROTOTYPE_SAVE` to the path of an appropriate old save to
repeat this optional test; it is skipped when no path is supplied.

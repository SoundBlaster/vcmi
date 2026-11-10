# Town appearance and an anchor outside the map rectangle

`CTownInstanceConstructor::randomizeObject` chooses an appearance override
using terrain at the town's anchor. A wrapped static object can have an
anchor outside the canonical rectangle even when its footprint is valid.
With a real town in a synthetic game state, calling the constructor with the
anchor shifted by one map period on each axis raised
`CallbackInvalidCallException` before the fix.

For toroidal maps the constructor now canonicalizes the anchor for the
terrain lookup, leaving the town's anchor unchanged. An invalid level still
reaches the existing callback validation. Bounded maps retain the original
lookup and error behavior.

`HeroMovementTest.TownAppearanceUsesWrappedAnchorTerrain` checks the real
constructor for a horizontal shift, a vertical shift, a corner shift and two
periods in both axes; the selected appearance matches the original and a
bounded-map out-of-range lookup still fails. It invokes the constructor
directly and does not establish that every way of creating or loading a
seam-crossing town has been exercised.

The macOS arm64 client, server, editor and test targets built successfully.
The full test suite passed 1,432 tests, skipped three optional external-input
tests and reported five disabled tests. The log is
`full-suite-town-anchor-wrap.log` in the checkout-specific TB5 build root.
The linker reported duplicate libraries and missing Conan search paths but
completed successfully.

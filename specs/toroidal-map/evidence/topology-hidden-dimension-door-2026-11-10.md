# Hidden terrain and Dimension Door planning

In a synthetic 36×36 toroidal game, the player callback's
`DimensionDoorEffect::isValidTargetFrom` changed from true to false when an
unseen target across the seam changed from grass to water. The pre-fix
regression test failed on the hidden-water expectation. Its unrestricted
server callback also distinguished the actual terrain, as it must when
executing the spell. This reproduces a callback-level knowledge leak; it is
not by itself evidence of a specific rendered cursor or a network leak.

For toroidal maps, target preview now treats an unknown destination as
speculative when the spell permits fog targeting, and rejects it when fog
targeting is disabled. Known blocked destinations are rejected; a known
destination with an unknown hypothetical source is not evaluated using the
hidden source terrain. Unrestricted server callbacks continue to check the
actual terrain. The bounded-map behavior was checked separately and retained.

Nullkiller2's post-cast continuation estimate also read hidden source and
neighbor terrain. It now uses player-visible tiles on toroidal maps. A prior
AI fixture demanded that the AI choose the only hidden grass cell among hidden
rocks. The updated fixture still checks a planned crossing across the seam,
and checks that replacing hidden rocks with grass does not change its selected
landing. A blind target is speculative and can still fail when executed; this
change does not guarantee AI success or audit every AI knowledge source.

On macOS arm64, source `f99e66e` plus these changes built `vcmitest`,
`vcmiclient` and `vcmiserver`. Eight focused spell and AI tests passed. The
first full run, after the preview change but before correcting the AI
continuation and fixture, failed the old hidden-grass expectation. After the
full change, 1,453 enabled tests passed and three optional-input tests were
skipped; five tests were disabled, and no TCP test was excluded. The full log
is `full-suite-dd-knowledge-final.log` under the VCMI TB5 build root. Linker
warnings about duplicate libraries and absent Conan search paths remain.
No real GUI spell cast or multiplayer confidentiality check was performed.

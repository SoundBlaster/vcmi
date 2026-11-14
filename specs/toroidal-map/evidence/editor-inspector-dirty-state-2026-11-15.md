# Inspector population and unsaved state

With the editor at `105f539`, opening the saved seam test map and selecting
its red town marked the window as modified, even without a property edit.
Closing then prompted about unsaved changes. Inspector rebuilds its table
programmatically, and the table's change handler treats checkable item
changes as user edits.

`Inspector::updateProperties` now blocks the table's signals while it
repopulates the properties; the previous signal state is restored on return.
This applies to subsequent programmatic refreshes as well as initial
selection. User edits still use the ordinary table change handler.

The editor built on macOS arm64 from `105f539` plus the change
(`inspector.cpp` SHA-256
`e226a80e7e121c3210dadc6ef361587c2d9ab767e301970af81c072c8f4a010f`).
In a signed editor using a separate test copy, selecting `town_0` no longer
added the modified marker. Changing its owner from red to neutral did add the
marker. Saving displayed a generic map-validation warning but wrote the copy;
after closing and reopening, Inspector showed the owner as neutral. The
validation issues were not individually diagnosed. The original playable
demo file was unchanged; the modified test copy is not asserted playable.
This checks one property and one Qt/macOS environment, not every delegate or
platform.

# Hover on repeated city copies

On macOS arm64, the signed client built from `46706c2` plus the status-bar
correction subsequently committed as `803bf77` ran the toroidal fixture
«Toroidal Water Vertical Crossing». Eight keypad-minus presses showed several
periodic copies of the 36×36 map. With the diagnostic hover command, the
upper-middle city copy displayed the name and ownership of the red player's
town. Hovering fog cleared the text; hovering the lower-middle city copy then
displayed the same name and ownership. After another fog hover cleared the
text, the lower-right city copy displayed the same description.

This is one object, one viewport and one zoom. The intervening fog checks
exclude simply retaining the prior status text for the observed sequences.
They do not verify clicks, the cursor graphic, other object types or every
periodic copy; the separate [wide-zoom record](topology-gui-wide-zoom-2026-11-10.md)
covers a horizontal click pair.

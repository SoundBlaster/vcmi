# Clear stale map hover text

On the uncorrected client, hovering a visible object and then an unseen tile
left the object's name in the status bar. The corresponding early return was
also present in the baseline `30e9207`; this was not introduced by toroidal
topology. Moving outside a bounded map had an analogous early return.

The client now clears the status bar when the hovered tile is unseen or when
there is no tile under the pointer. On a signed macOS arm64 client built from
`46706c2` plus this correction, the diagnostic hover command was used for
these interactive checks:

* On the bounded map «Миротворец», hovering a visible town displayed its name;
  hovering fog cleared it. Repeating the town hover and then moving outside the
  map also cleared it.
* On the toroidal fixture «Toroidal Water Vertical Crossing», hovering visible
  water displayed «Вода»; hovering fog cleared it.

The checks establish the visible status-bar behavior for those sequences, not
the behavior of the cursor graphic or all spell and input combinations.

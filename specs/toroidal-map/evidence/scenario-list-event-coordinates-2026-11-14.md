# Scenario list event coordinates

The scenario list formerly resolved a click by reading the current global
cursor position, although its handler already received the event position.
The diagnostic command channel reproduced a mismatch between those values;
this demonstrated wrong-row selection in that channel, not a reproduced
ordinary-mouse failure. Double-click and popup handlers had the same mismatch
and could index the list before rejecting an invalid row.

The three handlers now resolve the row using the coordinates of their own
event. Double-click and popup handling reject an invalid row before indexing.
This change applies to ordinary scenario-list input as well as diagnostic input.

On macOS arm64, the client built on TB5 from `8552be9` plus this change and
passed deep strict signature verification. In the running client, sorting the
scenario list and selecting the fourth row through the diagnostic channel
updated the selection and detail pane correctly. Attempts to inject a native
double click through Computer Use did not start the scenario; the cause was
not established. The double-click and popup interactions therefore remain
without a successful interactive regression check, and no ordinary-mouse crash
or wrong-row selection was reproduced in this run.

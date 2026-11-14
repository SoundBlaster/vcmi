# Editor tooltip and author documentation

The General settings tooltip incorrectly stated that static objects cannot
cross an edge. Static footprints fitting one map period can now wrap, subject
to the writer and overlap restrictions. The English tooltip and its Russian
translation now direct authors to check overlapping objects at seams.

The map editor and file-format guides describe the experimental opt-in,
branch-local VMAP marker, compatibility and current authoring restrictions.
The macOS arm64 `vcmieditor` target built on TB5, regenerating the UI header
and Russian translation. XML parsing also confirmed that the tooltip source
matches exactly one non-empty Russian translation. This verifies compilation
and source consistency; the updated tooltip was not inspected in a running
editor.

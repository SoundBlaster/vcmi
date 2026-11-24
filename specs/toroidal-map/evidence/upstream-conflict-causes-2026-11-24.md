# Why the VCMI integration conflicts

Source audit of fork `21f7a4f9c` and upstream `43012e717`, with merge base
`30e920727`. The current merge initially reported 12 files. The timer and
test list are already staged as resolved. The other ten contain 12 conflict
hunks. The upstream revision includes 160 commits after the fork's
`develop` at `141d00369`, including 33 first-parent commits.

Git reports textual overlap. The source history explains why the overlap
exists. The following proposals still need integration tests.

| Files | What changed and why it overlaps | Maintenance decision to try |
| --- | --- | --- |
| `CMap.cpp`, `ObjectTemplate.cpp` | The fork used wrapped displacement for approaches (`6389865b3`) and honored eight-way template masks (`92e8cd55f`). Upstream `08ec247fc` removed that mask from the template and moved the north-side restriction to object types. Its legacy mod fallback reduces template masks to whether any template permits a northern approach. | Compute the approach vector through `MapTopology`. Apply object visit policy separately, using upstream's object-type model. Assess any fork-specific eight-way behavior explicitly before deciding what to retain. Review guard checks, pathfinder, server, RMG and asymmetric fixtures. |
| `CMinimap.cpp`, `settings.json` | Upstream `55a9a86b6` added towns to make them easier to see on large maps and replaced `minimapShowHeroes` with `minimapShowIcons`. The fork added pixel alignment, a scaled terrain raster and wrap copies, while `98863b9e6` added `globeCurvature` on the same schema line. | Use upstream's icon selection and visibility as input to the fork's pixel/copy mapping. Keep geometry in the existing helpers, retain the globe option and check old settings, fog and incremental repaint. Consider one schema entry per line only when the touched block warrants it. |
| `ESerializationVersion.h` | Both branches appended to an implicit shared enum. The fork assigned 940–946 to topology/state/framing changes. Upstream assigned 940–943 to HotA difficulty restrictions, lobby campaign loading, shared campaign data, and the object-type visit rule. | For this beta, assign the new range above both recorded maxima so old tagged beta ordinals cannot enter it. Validate the save tag and version together before reading layout-dependent fields. Review old untagged prototypes and stock saves separately. Check the lobby minimum and greeting/packet layout in both directions. |
| `mainwindow.cpp` | The fork's `8b680f353` changed the log path in the GUI constructor. Upstream `8ec43e06a` extracted logging and startup into `EditorStartup` so `--generate` can work without a window. | Move the small log override to the shared startup boundary. Verify both normal startup and headless generation, and retain the separate PNG feature. |
| `MapFormatH3M.cpp` | The fork's `34e386af3` catches a missing predefined HotA hero. Upstream `38f5faea6` changed the nearby handler API to return a reference. | Retain the check through the new API if the regression fixture still needs it. Keep this as a compatibility fix that can be retired separately from the torus. |
| `JsonBonus.cpp`, `CGeneralTextHandler.cpp` | Both branches accepted the same bonus aliases (`9ce0ab99b`, `db9f731d6`) and fixed the same hardcoded language-profile count (`a8d5132e8`, `7d976505b`). Upstream also checks that the language and footprint lists have the same size. | Prefer the equivalent upstream implementation once its semantics and tests are confirmed. Keep fork tests that test a distinct behavior. |
| `russian.json`, `test/CMakeLists.txt`, `TurnTimerHandler.cpp` | Weblate edits landed beside fork strings, both branches added tests at one list position, and Git reported a blank-line overlap next to independent timer fixes. The upstream timer fix handles games with fewer than eight players. The fork's logic handles transient movement at turn expiry. | Keep distinct keys, test targets and timer behavior. Validate unique locale keys, test discovery and the timer cases. Avoid broad formatting churn. |

## Where the architecture matters

The fork already has a geometry boundary in `MapTopology`. World rules still
need that boundary in several places, including pathfinding, fog, rendering
and the editor. `CMap::checkForVisitableDir` is a high-value point to keep
small because it serves movement and guarding, and currently combines relative
position with a lookup of the object's visit rule. The upstream extraction of
`EditorStartup` provides an existing boundary for an editor change. The
minimap already has pixel and position helpers. A rewrite would add a large
patch without addressing independent changes to versions or translation keys.

The save/network ordinal is a separate architecture concern. One ordered
integer currently represents several format and behavior decisions. A
version/tag validation point and a descriptive compatibility matrix reduce
the chance of decoding by the wrong rules. Renumbering this beta does not
create a permanent reservation against future upstream versions. Reassess a
separate fork protocol/format identity when a subsequent integration shows that
the existing tags and validation cannot distinguish the required layouts.

## Tasks for future updates

- [ ] At the start of work on a shared subsystem and before a beta cut, inspect
  upstream changes since our last integrated revision. While upstream and the
  fork are both active, try a small rehearsal merge roughly weekly, or as soon
  as a relevant engine refactor lands. Use `git merge-tree --write-tree
  --name-only` against pinned commits so the check is repeatable.
- [ ] Keep topology, incidental HotA/mod fixes, demo tooling and distribution
  changes in separate, identifiable commits. For an overlapping fix compare
  behavior and tests, then retire the duplicate if upstream fully covers it.
  Avoid an additional permanent branch for every individual patch.
- [ ] Prefer small calls into geometry helpers at the points where rules need
  them. Keep content rules and UI selection policy in their owning components.
  Test the same move, guard or icon update away from and across a seam, and
  include bounded-map behavior in the same verification pass.
- [ ] Keep a short compatibility review for updates that touch save versions,
  network packets, object policies or UI settings. Check serializers, setup
  and call sites even when Git merged them automatically. Save/peer fixtures
  stay revision-specific, and a successful rehearsal is checked separately
  from a build or actual gameplay.

These tasks refine INT-02, INT-04, INT-05 and INT-06 in the
[integration plan](../upstream-integration-plan-2026-11-24.md).

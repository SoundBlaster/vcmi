# AI neighborhood and attack direction: build record

On 2026-11-09 Nullkiller2's shared neighbor iteration and selected exploration
calculations were adapted to wrapped coordinates. The client attack direction
also uses the short displacement across a seam. Other AI calculations,
adventure spells, and complete turn behavior still require an audit and
runtime verification; no AI profile is claimed as fully supported.

Source baseline: `31fbbef` plus the uncommitted files identified below. The
Release build of `vcmitest`, `vcmiclient`, `vcmiserver` and `vcmieditor`
completed with exit 0 on macOS arm64 using the same TB5 build root as the unit
record; the log is `ai-build.log` there. The linker repeated warnings about
duplicate system libraries and absent Conan system-library search paths.

`Nullkiller2_AIUtility.neighbourEnumerationWrapsAtCorners` was compiled but
not executed. The preceding integration-test attempts fail in the same global
setup because `DATA/LCDESC` from the original game is unavailable. Build
success is not evidence of AI planning, visual direction, or gameplay.

```text
ac27107956b6ea07afd07966e72449c690451bef033207a069d9bd1a72edfb73  AI/Nullkiller2/AIUtility.h
61d093773093d4cb176a69f6cf4c96d38114b5470c8031992069108a807745b9  AI/Nullkiller2/Behaviors/ExplorationBehavior.cpp
0dbc52dc1e574d22a2444c1364847755662b9fa0689df920eda5abe413e438af  AI/Nullkiller2/Helpers/ExplorationHelper.cpp
836a0ccd1a0a7aa3122f53ecc574135864ed6480a427073481d91b19dc11f09a  client/HeroMovementController.cpp
7a340657ee85da409eebbd3f728ea04fc31efbb609258a69b1d8f8fc4eef62c9  test/nullkiller2/AIUtilityTest.cpp
```

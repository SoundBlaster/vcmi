# Validation record

Date: 2026-11-09. Working directory: VCMI repository root. Source baseline:
`30e9207276f137658da5132705c05644bfe33aee`; the inspected implementation
files were unchanged against HEAD at validation. The created specification
artifacts were untracked and local at this point.

| Command | Exit | Result |
| --- | ---: | --- |
| `specpm validate . --json` | 0 | `status: valid`; 0 errors, 0 warnings; checked manifest and BoundarySpec. |
| `hypercode parse specs/toroidal-map/architecture.hc` | 0 | Printed the expected `AdventureMapSession` composition. |
| `hypercode validate specs/toroidal-map/architecture.hc --hcs specs/toroidal-map/architecture.hcs` | 0 | `ok: no issues found`. |

The initial SpecPM validation reported one missing-evidence warning because
`review.md` had not yet been written; after adding it, the final run above
reported no warnings. These checks establish supported structural and syntax
properties only. No application build, gameplay test, visual test, AI test,
save/load test, or multiplayer test was run.

Tool identities:

- `specpm 0.2.0`, `/opt/homebrew/bin/specpm` SHA-256
  `1c80370f940afd1be509c5bd53fb0c7c6d1bd558d8e6718fdf19b9179ae09142`;
  installed Python module `specpm/cli.py` SHA-256
  `ad74bb922cb3133626279443f06a65a05864d6b398791d52fe501d5886655f95`,
  `specpm/core.py` SHA-256
  `af4ac664e47283d1175473a65cc03779ee5ed62b6bccc8a57ff4b2ca97718528`.
  The build/source revision of this installation is unknown.
- Hypercode binary
  `/Users/egor/Development/GitHub/0AL/Hypercode/.build/release/hypercode`,
  SHA-256 `ed236f86551590d3b0539c66e572cdfcdcf0ce567fa3913e2f2dfa0097dfc78b`.
  Its build/source revision is unknown.

Validated artifact identities:

```text
cd52cd29170931ed06f08ff2293333bc8b1adb6990a5b36b373908cea393302d  specpm.yaml
8f6d07e0ceb3aa4a2f0df1e7ac34e4cd5277245760a548124d625b9e5dcba65e  specs/toroidal-map.spec.yaml
d728846b82483e64c1b5407354fac85c87847f7719ee379ebb9df4a5bfccc9f6  specs/toroidal-map/architecture.hc
aa3a0f2adf7e4226bfc10fbe42b61b3c0f681343b5ef891127c47c25ede6f6a6  specs/toroidal-map/architecture.hcs
9acd7d2956ea748578d73a2430e084c8a33aaa776f37a8fbcd060205ea6e2b7f  specs/toroidal-map/review.md
```

## Implementation matrix update

On 2026-11-09, after adding the tracking matrix and scenarios, using the same
repository root and SpecPM installation:

| Check | Exit | Full diagnostic |
| --- | ---: | --- |
| `specpm validate . --json` | 0 | `status: valid`, `error_count: 0`, `warning_count: 0`. |
| Requirement and scenario ID cross-check below | 0 | `requirements 14 missing [] unknown []`; `scenarios 24 undefined references [] unmapped []`. |

The ID cross-check checks identifier presence only; it does not prove that a
scenario sufficiently tests the associated requirement or that it ran.

```sh
/opt/homebrew/opt/python@3.10/bin/python3.10 -c 'import re,yaml,pathlib; s=yaml.safe_load(pathlib.Path("specs/toroidal-map.spec.yaml").read_text()); m=pathlib.Path("specs/toroidal-map/implementation-matrix.md").read_text(); r=pathlib.Path("specs/toroidal-map/review.md").read_text(); req={v["id"] for v in s["constraints"]}; listed=set(re.findall(r"\| `(\w+)` \|",m)); meta={"evidence_not_implementation"} if "`evidence_not_implementation`" in m else set(); scenarios=set(re.findall(r"^\| ([GVPAE]\d+) \|",r,re.M)); refs=set(re.findall(r"\b[GVPAE]\d+\b",m)); print("requirements",len(req),"missing",sorted(req-listed-meta),"unknown",sorted(listed-req)); print("scenarios",len(scenarios),"undefined references",sorted(refs-scenarios),"unmapped",sorted(scenarios-refs)); raise SystemExit(bool(req-listed-meta or listed-req or refs-scenarios or scenarios-refs))'
```

Artifact SHA-256 after this update:

```text
cd52cd29170931ed06f08ff2293333bc8b1adb6990a5b36b373908cea393302d  specpm.yaml
29993b014a8f299498666d3b490a2bdf72566580e61d12eaff5992c6c57315be  specs/toroidal-map.spec.yaml
d728846b82483e64c1b5407354fac85c87847f7719ee379ebb9df4a5bfccc9f6  specs/toroidal-map/architecture.hc
aa3a0f2adf7e4226bfc10fbe42b61b3c0f681343b5ef891127c47c25ede6f6a6  specs/toroidal-map/architecture.hcs
dfd88da5dbdb0f6120c2895a9e99cd628021e20a45f03af3b3c3a23cdcaba8ca  specs/toroidal-map/review.md
bad9b1de5ca83067d9def627d4a77452fe1bae1da185533438951fa33cc2ccc6  specs/toroidal-map/implementation-matrix.md
```

#!/usr/bin/env python3
"""Check identifiers in the toroidal spec, review scenarios and matrix.

This checks references, not whether the cited tests prove each requirement.
"""

import re
from pathlib import Path

import yaml


SCENARIO = re.compile(r"\b[GVPAE]\d+\b")
SCENARIO_RANGE = re.compile(r"\b([GVPAE])(\d+)[–-]([GVPAE])(\d+)\b")


def scenario_references(text):
    def expand(match):
        first, start, last, end = match.groups()
        start, end = int(start), int(end)
        if first != last or start > end or end - start > 1000:
            raise ValueError(f"Invalid scenario range: {match.group(0)}")
        return " ".join(f"{first}{number}" for number in range(start, end + 1))

    return set(SCENARIO.findall(SCENARIO_RANGE.sub(expand, text)))


def main():
    root = Path(__file__).resolve().parents[3]
    spec = yaml.safe_load((root / "specs/toroidal-map.spec.yaml").read_text())
    matrix = (root / "specs/toroidal-map/implementation-matrix.md").read_text()
    review = (root / "specs/toroidal-map/review.md").read_text()
    requirements = {entry["id"] for entry in spec["constraints"]}
    rows = set(re.findall(r"\| `(\w+)` \|", matrix))
    # The evidence-only meta constraint is discussed outside the main table.
    if "`evidence_not_implementation`" in matrix:
        rows.add("evidence_not_implementation")
    scenarios = set(re.findall(r"^\| ([GVPAE]\d+) \|", review, re.M))
    references = scenario_references(matrix)
    missing, unknown = requirements - rows, rows - requirements
    undefined, unmapped = references - scenarios, scenarios - references
    print(f"requirements {len(requirements)} missing {sorted(missing)} unknown {sorted(unknown)}")
    print(f"scenarios {len(scenarios)} undefined {sorted(undefined)} unmapped {sorted(unmapped)}")
    return int(bool(missing or unknown or undefined or unmapped))


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Compare active-cell overlaps in a two-level source and toroidal draft.

This is a geometry diagnostic, not a determination of in-game accessibility.
Both inputs are read-only; no map data is included with this tool.
"""

import argparse
import collections
import json
import zipfile

from analyze_stacked_map import load_json, occupants


def audit(source, draft, source_size, draft_size):
    sw, sh = source_size
    dw, dh = draft_size
    if min(sw, sh, dw, dh) <= 0:
        raise ValueError("Map dimensions must be positive")
    for objects in (source, draft):
        names = [obj["instanceName"] for obj in objects]
        if len(names) != len(set(names)):
            raise ValueError("Object instance names must be unique")
    _, old_interactions, clipped = occupants(source, sw, sh, False)
    old = {tuple(sorted((first[0], second[0]))) for _, first, second in old_interactions}
    cells = collections.defaultdict(list)
    for obj in draft:
        if obj["l"] != 0:
            raise ValueError("Draft must have one level")
        mask = obj["template"]["mask"]
        for row, line in enumerate(mask):
            for col, kind in enumerate(line):
                if kind in "BHAT":
                    position = ((obj["x"] - len(line) + 1 + col) % dw,
                                (obj["y"] - len(mask) + 1 + row) % dh)
                    cells[position].append((obj["instanceName"], obj["type"], kind))
    found = collections.defaultdict(list)
    for (x, y), entries in cells.items():
        for i, first in enumerate(entries):
            for second in entries[i + 1:]:
                if first[0] == second[0] or (first[2] not in "AT" and second[2] not in "AT"):
                    continue
                pair = tuple(sorted((first[0], second[0])))
                if pair not in old:
                    found[pair].append({"cell": [x, y], "objects": [list(first), list(second)]})
    return {"source_clipped_active_cells": clipped,
            "new_interactive_pairs": len(found),
            "pairs": [{"names": list(pair), "overlaps": sorted(items, key=lambda v: v["cell"])}
                      for pair, items in sorted(found.items())]}


def read_map(path):
    with zipfile.ZipFile(path) as archive:
        return load_json(archive, "header.json"), load_json(archive, "objects.json")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", help="Two-level source VMAP")
    parser.add_argument("draft", help="Single-level toroidal draft VMAP")
    args = parser.parse_args()
    source_header, source = read_map(args.source)
    draft_header, draft = read_map(args.draft)
    if draft_header.get("topology") != "toroidal":
        raise ValueError("Draft must explicitly declare toroidal topology")
    source_levels = sorted(source_header["mapLevels"].values(), key=lambda level: level["index"])
    draft_levels = list(draft_header["mapLevels"].values())
    if len(source_levels) != 2 or [level["index"] for level in source_levels] != [0, 1] or len(draft_levels) != 1 or draft_levels[0]["index"] != 0:
        raise ValueError("Expected two source levels and one draft level")
    size = lambda level: (level["width"], level["height"])
    if size(source_levels[0]) != size(source_levels[1]):
        raise ValueError("Source levels have different sizes")
    print(json.dumps(audit(source, draft, size(source_levels[0]), size(draft_levels[0])), indent=2))


if __name__ == "__main__":
    main()

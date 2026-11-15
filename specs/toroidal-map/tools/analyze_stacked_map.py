#!/usr/bin/env python3
"""Report footprint overlaps for a proposed two-level VMAP rearrangement.

The second level is rotated 180 degrees above the first, retaining upright
object templates. This is a geometry estimate, not a map converter or an
engine validation. The input is never modified.
"""

import argparse
import collections
import json
import zipfile


def load_json(archive, name):
    # VCMI's writer emits standalone scope-comment lines in objects.json.
    text = archive.read(name).decode("utf-8")
    return json.loads("\n".join(line for line in text.splitlines()
                                if not line.lstrip().startswith("//")))


def occupants(objects, width, height, stacked, placement="anchor"):
    cells = collections.defaultdict(list)
    clipped = 0
    for obj in objects:
        x, y, level = obj["x"], obj["y"], obj["l"]
        if level not in (0, 1):
            raise ValueError(f"Unsupported level {level}")
        mask = obj["template"]["mask"]
        if stacked:
            if level == 1:
                x, y = width - 1 - x, height - 1 - y
                if placement == "visitable":
                    visit_offsets = [(len(line) - 1 - col, len(mask) - 1 - row)
                                     for row, line in enumerate(mask)
                                     for col, kind in enumerate(line) if kind in "AT"]
                    if visit_offsets:
                        # The first entrance keeps its rotated cell. Other
                        # entrances and the rest of the mask may still move.
                        dx, dy = visit_offsets[0]
                        x += 2 * dx
                        y += 2 * dy
                    else:
                        x += max((len(line) for line in mask), default=1) - 1
                        y += len(mask) - 1
            else:
                y += height
            level = 0
        for row, line in enumerate(mask):
            for col, kind in enumerate(line):
                # Mask rows and columns end at the object anchor.
                if kind not in "BHAT":
                    continue
                cell_x = x - (len(line) - 1 - col)
                cell_y = y - (len(mask) - 1 - row)
                if stacked:
                    cell_x %= width
                    cell_y %= height * 2
                elif not (0 <= cell_x < width and 0 <= cell_y < height):
                    clipped += 1
                    continue
                cells[(cell_x, cell_y, level)].append(
                    (obj["instanceName"], kind, obj["type"])
                )

    pairs, interactions = set(), []
    for position, entries in cells.items():
        for i, first in enumerate(entries):
            for second in entries[i + 1:]:
                if first[0] == second[0]:
                    continue
                pair = tuple(sorted((first[0], second[0])))
                pairs.add(pair)
                if first[1] in "AT" or second[1] in "AT":
                    interactions.append((position, first, second))
    return pairs, interactions, clipped


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vmap", help="Read-only source VMAP with exactly two levels")
    parser.add_argument("--placement", choices=("anchor", "visitable"), default="anchor",
                        help="Rotate anchors or preserve the first visitable cell")
    args = parser.parse_args()
    with zipfile.ZipFile(args.vmap) as archive:
        header = load_json(archive, "header.json")
        objects = load_json(archive, "objects.json")

    levels = sorted(header["mapLevels"].values(), key=lambda level: level["index"])
    if len(levels) != 2 or [level["index"] for level in levels] != [0, 1]:
        raise ValueError("Expected exactly two indexed levels")
    width, height = levels[0]["width"], levels[0]["height"]
    if width <= 0 or height <= 0 or (levels[1]["width"], levels[1]["height"]) != (width, height):
        raise ValueError("Expected positive, equally sized levels")
    names = [obj["instanceName"] for obj in objects]
    if len(set(names)) != len(names):
        raise ValueError("Object instance names must be unique")

    original, original_visits, clipped = occupants(objects, width, height, False)
    stacked, stacked_visits, _ = occupants(objects, width, height, True, args.placement)
    new = stacked - original
    new_visits = [item for item in stacked_visits
                  if tuple(sorted((item[1][0], item[2][0]))) in new]
    result = {
        "dimensions": [width, height],
        "placement": args.placement,
        "objects": len(objects),
        "original_blocking_pairs": len(original),
        "original_interaction_occurrences": len(original_visits),
        "original_clipped_blocking_cells": clipped,
        "stacked_blocking_pairs": len(stacked),
        "stacked_interaction_occurrences": len(stacked_visits),
        "new_blocking_pairs": len(new),
        "removed_blocking_pairs": len(original - stacked),
        "new_interaction_occurrences": len(new_visits),
        "new_interaction_examples": new_visits[:8],
    }
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()

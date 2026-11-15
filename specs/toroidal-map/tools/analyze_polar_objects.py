#!/usr/bin/env python3
"""Estimate which second-level objects touch a snow island in a stacked preview."""

import argparse
import collections
import json
import zipfile

from analyze_stacked_map import load_json
from preview_stacked_terrain import snow_component, stack_levels


def analyze_objects(objects, component, width, level_height):
    if any(not (0 <= x < width and 0 <= y < level_height) for x, y in component):
        raise ValueError("Snow component must stay within the rotated second level")

    def on_island(x, y):
        return (width - 1 - x, level_height - 1 - y) in component

    anchors, footprints, visitable = [], [], []
    for obj in objects:
        if obj["l"] != 1:
            continue
        x, y = obj["x"], obj["y"]
        if on_island(x, y):
            anchors.append(obj)
        mask = obj["template"]["mask"]
        touched = False
        visited = False
        for row, line in enumerate(mask):
            for col, kind in enumerate(line):
                if kind not in "VBHAT":
                    continue
                cell_x = x - (len(line) - 1 - col)
                cell_y = y - (len(mask) - 1 - row)
                if on_island(cell_x, cell_y):
                    touched = True
                    visited |= kind in "AT"
        if touched:
            footprints.append(obj)
        if visited:
            visitable.append(obj)

    return {
        "snow_cells": len(component),
        "object_anchors": len(anchors),
        "object_footprints": len(footprints),
        "objects_with_visitable_cells": len(visitable),
        "visitable_types": dict(sorted(collections.Counter(obj["type"] for obj in visitable).items())),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vmap")
    parser.add_argument("x", type=int, help="Snow seed x in the stacked preview")
    parser.add_argument("y", type=int, help="Snow seed y in the stacked preview")
    args = parser.parse_args()
    with zipfile.ZipFile(args.vmap) as archive:
        surface = load_json(archive, "surface_terrain.json")
        underground = load_json(archive, "underground_terrain.json")
        objects = load_json(archive, "objects.json")
    rows = stack_levels(surface, underground)
    component = snow_component(rows, (args.x, args.y))
    print(json.dumps(analyze_objects(objects, component, len(rows[0]), len(underground)), indent=2))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Estimate which second-level objects touch an east-edge land fragment."""

import argparse
import json
import zipfile

from analyze_polar_objects import analyze_objects
from analyze_stacked_map import load_json
from preview_stacked_terrain import east_land_fragment


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vmap")
    parser.add_argument("x", type=int, help="Fragment seed x on the original second level")
    parser.add_argument("y", type=int, help="Fragment seed y on the original second level")
    args = parser.parse_args()
    with zipfile.ZipFile(args.vmap) as archive:
        underground = load_json(archive, "underground_terrain.json")
        objects = load_json(archive, "objects.json")
    component = east_land_fragment(underground, (args.x, args.y))
    width, height = len(underground[0]), len(underground)
    rotated = {(width - 1 - x, height - 1 - y) for x, y in component}
    result = analyze_objects(objects, rotated, width, height)
    result["fragment_cells"] = result.pop("snow_cells")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()

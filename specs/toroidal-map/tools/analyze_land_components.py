#!/usr/bin/env python3
"""Inspect terrain-only land connectivity on a toroidal surface VMAP."""

import argparse
import collections
import json
import zipfile

from analyze_stacked_map import load_json


def parse_point(value):
    try:
        parts = value.split(",")
        if len(parts) != 2:
            raise ValueError
        return tuple(int(part) for part in parts)
    except ValueError as error:
        raise argparse.ArgumentTypeError("expected X,Y") from error


def inspect(path, points):
    with zipfile.ZipFile(path) as archive:
        header = load_json(archive, "header.json")
        rows = load_json(archive, "surface_terrain.json")
    if header.get("topology") != "toroidal" or not rows or not rows[0]:
        raise ValueError("expected a non-empty toroidal map")
    height, width = len(rows), len(rows[0])
    if any(len(row) != width for row in rows):
        raise ValueError("terrain rows have inconsistent widths")
    known = {"wt", "sn", "sa", "hl", "gr", "sw", "lv", "rg", "dt", "ws", "sb"}
    if any(not isinstance(cell, str) or cell[:2] not in known for row in rows for cell in row):
        raise ValueError("unrecognized terrain; update the classifier before analysis")
    if any(not (0 <= x < width and 0 <= y < height) or rows[y][x].startswith("wt") for x, y in points):
        raise ValueError("all points must be in-bounds land cells")
    components = []
    assigned = set()
    for start in points:
        if start in assigned:
            continue
        visited = {start}
        queue = collections.deque([start])
        while queue:
            x, y = queue.popleft()
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    if not (dx or dy):
                        continue
                    neighbor = ((x + dx) % width, (y + dy) % height)
                    if neighbor not in visited and not rows[neighbor[1]][neighbor[0]].startswith("wt"):
                        visited.add(neighbor)
                        queue.append(neighbor)
        members = [point for point in points if point in visited]
        assigned.update(members)
        components.append({"tiles": len(visited), "points": members})
    return {"width": width, "height": height, "components": components}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("map")
    parser.add_argument("points", nargs="+", type=parse_point, metavar="X,Y")
    args = parser.parse_args()
    print(json.dumps(inspect(args.map, args.points), indent=2))

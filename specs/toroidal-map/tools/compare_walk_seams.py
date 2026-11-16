#!/usr/bin/env python3
"""Compare bounded components with additional wrapped neighbor edges.

Input is an engine-produced grid: '.' is an eligible cell, '#' excluded.
Eight-neighbor connectivity is a geometric approximation, not hero pathfinding.
"""
import argparse
from collections import deque
import json


def compare(rows, zones=None):
    if not rows or not rows[0] or any(len(row) != len(rows[0]) for row in rows):
        raise ValueError("grid must be nonempty and rectangular")
    if any(c not in ".#" for row in rows for c in row):
        raise ValueError("grid contains an unexpected cell")
    h, w = len(rows), len(rows[0])
    if zones is not None and (len(zones) != h or any(len(row) != w for row in zones)):
        raise ValueError("zone grid dimensions differ from walk grid")
    labels = {}
    sizes = []
    for y in range(h):
        for x in range(w):
            if rows[y][x] != "." or (x, y) in labels:
                continue
            index = len(sizes)
            queue = deque([(x, y)])
            labels[(x, y)] = index
            size = 0
            while queue:
                a, b = queue.popleft()
                size += 1
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        nx, ny = a + dx, b + dy
                        if (dx or dy) and 0 <= nx < w and 0 <= ny < h and rows[ny][nx] == "." and (nx, ny) not in labels:
                            labels[(nx, ny)] = index
                            queue.append((nx, ny))
            sizes.append(size)
    links = {}
    zone_links = {}
    for (x, y), source in labels.items():
        if x not in (0, w - 1) and y not in (0, h - 1):
            continue
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if not (dx or dy):
                    continue
                raw_x, raw_y = x + dx, y + dy
                if 0 <= raw_x < w and 0 <= raw_y < h:
                    continue
                target_pos = (raw_x % w, raw_y % h)
                target = labels.get(target_pos)
                if target is None:
                    continue
                if zones is not None:
                    first, second = zones[y][x], zones[target_pos[1]][target_pos[0]]
                    if first != second:
                        pair = tuple(sorted((first, second)))
                        zone_links.setdefault(pair, {"zones": pair,
                                                    "example": [[x, y], list(target_pos)],
                                                    "directed_edges": 0})
                        zone_links[pair]["directed_edges"] += 1
                if target == source:
                    continue
                pair = tuple(sorted((source, target)))
                links.setdefault(pair, {"components": pair,
                                        "sizes": [sizes[i] for i in pair],
                                        "example": [[x, y], list(target_pos)]})
    result = {"width": w, "height": h, "components": len(sizes),
              "eligible_cells": len(labels), "seam_links": list(links.values())}
    if zones is not None:
        result["zone_seam_links"] = list(zone_links.values())
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("grid")
    parser.add_argument("--zones", help="generation-time zone IDs, space-separated per row")
    args = parser.parse_args()
    with open(args.grid, encoding="utf-8") as stream:
        rows = [line.rstrip("\n") for line in stream]
    zones = None
    if args.zones:
        with open(args.zones, encoding="utf-8") as stream:
            zones = [[int(value) for value in line.split()] for line in stream]
    print(json.dumps(compare(rows, zones), indent=2))

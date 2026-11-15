#!/usr/bin/env python3
"""Assemble a diagnostic single-level VMAP; the result is not a playable demo.

This first-stage draft rotates the second level and keeps object artwork
upright. It does not resolve overlaps, repair roads or preserve cross-level
gate pairing. Never publish or install its output as a finished map.
"""

import argparse
import collections
import copy
import json
import re
import zipfile

from analyze_stacked_map import load_json, occupants
from preview_stacked_terrain import east_land_fragment, rejoin_east_land_fragment


TAGGED_TOROIDAL_VERSION = 0x54520004
CELL_PART = re.compile(r"([A-Za-z0-9]{2})([0-9]+)([_+|\-])")
ROTATED_FLIP = {"_": "+", "+": "_", "-": "|", "|": "-"}


def rotate_cell_art(cell):
    """Rotate serialized tile artwork 180 degrees, including its overlays.

    Road rendering spans adjacent rows; rotating these flags by itself does
    not prove continuity or preserve road movement semantics after editing.
    """
    parts = list(CELL_PART.finditer(cell))
    if not parts or len(parts) > 3 or "".join(part.group(0) for part in parts) != cell:
        raise ValueError(f"Invalid terrain cell: {cell!r}")
    return "".join(part.group(1) + part.group(2) + ROTATED_FLIP[part.group(3)] for part in parts)


def place_object(obj, width, height, fragment_objects=frozenset()):
    """Keep surface anchors; rotate second-level first visitable cells."""
    result = copy.deepcopy(obj)
    x, y, level = (obj[k] for k in ("x", "y", "l"))
    if level not in (0, 1):
        raise ValueError(f"Invalid object level: {obj.get('instanceName')!r}")
    if level == 0 or obj["instanceName"] in fragment_objects:
        result["x"], result["y"] = x % width, (y + height) % (2 * height)
    else:
        mask = obj["template"]["mask"]
        x, y = width - 1 - x, height - 1 - y
        entrances = [(len(line) - 1 - col, len(mask) - 1 - row)
                     for row, line in enumerate(mask)
                     for col, kind in enumerate(line) if kind in "AT"]
        if entrances:
            dx, dy = entrances[0]
            x, y = x + 2 * dx, y + 2 * dy
        else:
            x += max((len(line) for line in mask), default=1) - 1
            y += len(mask) - 1
        result["x"], result["y"] = x % width, y % (2 * height)
    result["l"] = 0
    return result


def fragment_object_names(objects, component):
    """Keep an entire object if an active footprint cell touches the fragment."""
    result = set()
    for obj in objects:
        if obj["l"] != 1:
            continue
        x, y = obj["x"], obj["y"]
        mask = obj["template"]["mask"]
        if any((x - (len(line) - 1 - col), y - (len(mask) - 1 - row)) in component
               for row, line in enumerate(mask) for col, kind in enumerate(line) if kind in "BHAT"):
            result.add(obj["instanceName"])
    return result


def blocking_pairs(objects, width, height):
    cells = collections.defaultdict(list)
    for obj in objects:
        mask = obj["template"]["mask"]
        for row, line in enumerate(mask):
            for col, kind in enumerate(line):
                if kind in "BHAT":
                    pos = ((obj["x"] - (len(line) - 1 - col)) % width,
                           (obj["y"] - (len(mask) - 1 - row)) % height)
                    cells[pos].append((obj["instanceName"], kind))
    pairs, interactions = set(), []
    for entries in cells.values():
        for index, first in enumerate(entries):
            for second in entries[index + 1:]:
                if first[0] == second[0]:
                    continue
                pair = tuple(sorted((first[0], second[0])))
                pairs.add(pair)
                if first[1] in "AT" or second[1] in "AT":
                    interactions.append(pair)
    return pairs, interactions


def nested_positions(value, path=()):
    if isinstance(value, dict):
        if {"x", "y", "l"} <= value.keys():
            yield path, value
        for key, item in value.items():
            yield from nested_positions(item, path + (key,))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            yield from nested_positions(item, path + (index,))


def assemble(header, surface, underground, objects, fragment_seed=None):
    levels = sorted(header["mapLevels"].values(), key=lambda level: level["index"])
    if len(levels) != 2 or [level["index"] for level in levels] != [0, 1]:
        raise ValueError("Expected exactly two indexed levels")
    width, height = levels[0]["width"], levels[0]["height"]
    if width <= 0 or height <= 0 or (levels[1]["width"], levels[1]["height"]) != (width, height):
        raise ValueError("Expected positive, equally sized levels")
    if any(len(rows) != height or any(len(row) != width for row in rows) for rows in (surface, underground)):
        raise ValueError("Terrain dimensions do not match the map header")
    if len({obj["instanceName"] for obj in objects}) != len(objects):
        raise ValueError("Object instance names must be unique")
    if any(list(nested_positions(obj.get("options"))) for obj in objects):
        raise ValueError("Object options contain coordinate references requiring manual handling")

    fragment = None
    moved = set()
    if fragment_seed is not None:
        component = east_land_fragment(underground, fragment_seed)
        moved = fragment_object_names(objects, component)
        surface, underground, fragment = rejoin_east_land_fragment(surface, underground, fragment_seed)
    converted = [place_object(obj, width, height, moved) for obj in objects]
    anchors = {}
    for old, new in zip(objects, converted):
        anchors.setdefault((old["x"], old["y"], old["l"]), []).append(new)

    new_header = copy.deepcopy(header)
    for path, position in nested_positions(new_header):
        if len(path) != 3 or path[0] != "players" or path[2] != "mainTown":
            raise ValueError(f"Unsupported coordinate reference in header: {path!r}")
        matches = [obj for obj in anchors.get((position["x"], position["y"], position["l"]), [])
                   if obj["type"] in ("town", "randomTown")]
        if len(matches) != 1:
            raise ValueError(f"Main town does not uniquely identify a town: {path!r}")
        position.update({k: matches[0][k] for k in ("x", "y", "l")})

    new_header["mapLevels"] = {"surface": {"height": 2 * height, "index": 0,
                                           "layer": "core:surface", "width": width}}
    new_header["versionMajor"] = TAGGED_TOROIDAL_VERSION
    new_header["topology"] = "toroidal"
    rows = [[rotate_cell_art(cell) for cell in reversed(row)] for row in reversed(underground)]
    rows += copy.deepcopy(surface)
    original, _, clipped = occupants(objects, width, height, False)
    assembled, interactions = blocking_pairs(converted, width, 2 * height)
    new = assembled - original
    return new_header, rows, converted, {
        "dimensions": [width, 2 * height], "objects": len(converted),
        "source_outside_anchors": sum(not (0 <= obj["x"] < width and 0 <= obj["y"] < height)
                                      for obj in objects),
        "source_clipped_blocking_cells": clipped,
        "new_blocking_pairs_estimate": len(new),
        "new_interaction_occurrences_estimate": sum(pair in new for pair in interactions),
        "east_fragment": fragment,
        "east_fragment_objects_moved": len(moved),
        "subterranean_gates": sum(obj["type"] == "subterraneanGate" for obj in objects),
        "limitations": ["diagnostic draft only", "object and interaction overlaps unresolved",
                        "road artwork and movement costs unverified", "cross-level gates unpaired",
                        "polar geography not adapted", "required mods not assessed"]
                       + (["east fragment not adapted"] if fragment is None
                          else ["east fragment object placement and overlaps unverified"]),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source")
    parser.add_argument("output", help="New draft VMAP; existing files are not overwritten")
    parser.add_argument("--rejoin-east-fragment", nargs=2, type=int, metavar=("X", "Y"),
                        help="Move one second-level east-edge land component and its objects to the surface")
    args = parser.parse_args()
    with zipfile.ZipFile(args.source) as archive:
        expected = {"header.json", "surface_terrain.json", "underground_terrain.json", "objects.json"}
        if not expected.issubset(archive.namelist()):
            raise ValueError("Missing source VMAP members")
        extras = {name: archive.read(name) for name in archive.namelist() if name not in expected}
        header, rows, objects, report = assemble(
            load_json(archive, "header.json"), load_json(archive, "surface_terrain.json"),
            load_json(archive, "underground_terrain.json"), load_json(archive, "objects.json"),
            args.rejoin_east_fragment)
    with zipfile.ZipFile(args.output, "x", compression=zipfile.ZIP_DEFLATED) as output:
        for name, data in extras.items():
            output.writestr(name, data)
        for name, value in (("header.json", header), ("surface_terrain.json", rows), ("objects.json", objects)):
            output.writestr(name, json.dumps(value, ensure_ascii=False, separators=(",", ":")))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

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
from preview_stacked_terrain import (add_illustrative_snow_island, east_land_fragment,
                                    oceanize_edge_rock, polarize_snow_island,
                                    rejoin_east_land_fragment, snow_component)


TAGGED_TOROIDAL_VERSION = 0x54520004
CELL_PART = re.compile(r"([A-Za-z0-9]{2})([0-9]+)([_+|\-])")
ROTATED_FLIP = {"_": "+", "+": "_", "-": "|", "|": "-"}


class NoClearPlacement(ValueError):
    pass


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


def polar_object_references(objects, component, width, height):
    """Select one source footprint cell for placing each polar object upright."""
    source = {(width - 1 - x, height - 1 - y) for x, y in component}
    references = {}
    for obj in objects:
        if obj["l"] != 1:
            continue
        mask = obj["template"]["mask"]
        candidates = []
        for row, line in enumerate(mask):
            for col, kind in enumerate(line):
                dx, dy = len(line) - 1 - col, len(mask) - 1 - row
                x, y = obj["x"] - dx, obj["y"] - dy
                if kind in "BHAT" and (x, y) in source:
                    candidates.append((x, y, dx, dy, kind))
        if candidates:
            # Prefer an entrance; otherwise use the active anchor when present.
            references[obj["instanceName"]] = min(candidates, key=lambda c: (c[4] not in "AT", c[2] != 0 or c[3] != 0))
    return references


def place_polar_object(obj, reference, component, width, source_height, result_height):
    """Map one selected cell to the stretched snow; never scale the sprite."""
    sx, sy, dx, dy, _ = reference
    rx, ry = width - 1 - sx, source_height - 1 - sy
    min_x = min(x for x, _ in component)
    max_x = max(x for x, _ in component)
    min_y = min(y for _, y in component)
    max_y = max(y for _, y in component)
    island_width = max_x - min_x + 1
    index = rx - min_x
    # polarize_snow_island samples source x = min_x + dest_x * island_width // width.
    left = (index * width + island_width - 1) // island_width
    right = ((index + 1) * width + island_width - 1) // island_width - 1
    dest_x = (left + right) // 2
    south_depth = (max_y - min_y + 1) // 2
    dest_y = (ry - min_y - south_depth) % result_height
    result = copy.deepcopy(obj)
    result.update(x=(dest_x + dx) % width, y=(dest_y + dy) % result_height, l=0)
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


def active_objects_on_cells(objects, cells, width, height):
    """Return object names and the subset with affected visitable cells."""
    touching, visitable = set(), set()
    for obj in objects:
        mask = obj["template"]["mask"]
        for row, line in enumerate(mask):
            for col, kind in enumerate(line):
                position = ((obj["x"] - (len(line) - 1 - col)) % width,
                            (obj["y"] - (len(mask) - 1 - row)) % height)
                if kind in "BHAT" and position in cells:
                    touching.add(obj["instanceName"])
                    if kind in "AT":
                        visitable.add(obj["instanceName"])
    return touching, visitable


def changed_cells(before, after):
    return {(x, y) for y, row in enumerate(before) for x, cell in enumerate(row) if cell != after[y][x]}


def relocate_decorations(objects, rows, names, max_distance=16, polar_references=None, allowed_types=None, reserved=()):
    """Move selected plain mountains to the nearest unoccupied dry cells."""
    width, height = len(rows[0]), len(rows)
    by_name = {obj["instanceName"]: obj for obj in objects}

    def footprint(obj, x, y):
        mask = obj["template"]["mask"]
        return {((x - (len(line) - 1 - col)) % width, (y - (len(mask) - 1 - row)) % height)
                for row, line in enumerate(mask) for col, kind in enumerate(line) if kind in "BHAT"}

    occupied = collections.defaultdict(set)
    for obj in objects:
        for cell in footprint(obj, obj["x"], obj["y"]):
            occupied[cell].add(obj["instanceName"])
    report = []
    if len(set(names)) != len(names):
        raise ValueError("Decoration names must not repeat")
    allowed_types = {"mountain"} if allowed_types is None else set(allowed_types)
    reserved = set(reserved)
    for name in names:
        obj = by_name.get(name)
        if not obj or obj["type"] not in allowed_types or obj.get("options"):
            raise ValueError(f"Expected a plain allowed decoration: {name!r}")
        if any(kind in "AT" for line in obj["template"]["mask"] for kind in line):
            raise ValueError(f"Interactive objects cannot be relocated as decorations: {name!r}")
        old_x, old_y = obj["x"], obj["y"]
        if not footprint(obj, old_x, old_y):
            raise ValueError(f"Mountain has no active footprint: {name!r}")
        offsets = sorted(((dx, dy) for dx in range(-max_distance, max_distance + 1)
                          for dy in range(-max_distance, max_distance + 1) if dx or dy),
                         key=lambda p: (max(abs(p[0]), abs(p[1])), p[0] ** 2 + p[1] ** 2, p[1], p[0]))
        selected = None
        for dx, dy in offsets:
            x, y = (old_x + dx) % width, (old_y + dy) % height
            if (x, y) == (old_x, old_y):
                continue
            cells = footprint(obj, x, y)
            if cells & reserved:
                continue
            if any(occupied[cell] - {name} for cell in cells):
                continue
            if any(rows[cy][cx][:2] in ("wt", "rc") or len(list(CELL_PART.finditer(rows[cy][cx]))) != 1
                   for cx, cy in cells):
                continue
            if polar_references and name in polar_references:
                _, _, ref_dx, ref_dy, _ = polar_references[name]
                if rows[(y - ref_dy) % height][(x - ref_dx) % width][:2] != "sn":
                    continue
            selected = x, y, cells
            break
        if selected is None:
            raise NoClearPlacement(f"No clear dry placement for {name!r} within {max_distance} cells")
        x, y, cells = selected
        for cell in footprint(obj, old_x, old_y):
            occupied[cell].discard(name)
        obj["x"], obj["y"] = x, y
        for cell in cells:
            occupied[cell].add(name)
        report.append({"object": name, "from": [old_x, old_y], "to": [x, y]})
    return report


def repair_mountain_interactions(source, converted, rows, source_width, source_height, polar_references=None):
    """Propose bounded mountain moves for new interactive footprint overlaps."""
    _, old_interactions, _ = occupants(source, source_width, source_height, False)
    old_interaction_pairs = {tuple(sorted((first[0], second[0]))) for _, first, second in old_interactions}
    prior_interactive = {entry[0] for _, first, second in old_interactions for entry in (first, second)}
    _, interactions = blocking_pairs(converted, len(rows[0]), len(rows))
    by_name = {obj["instanceName"]: obj for obj in converted}
    candidates = sorted({name for pair in interactions if pair not in old_interaction_pairs for name in pair
                         if by_name[name]["type"] == "mountain" and name not in prior_interactive
                         and not by_name[name].get("options")
                         and not any(kind in "AT" for line in by_name[name]["template"]["mask"] for kind in line)})
    moved, skipped = [], []
    for name in candidates:
        try:
            moved.extend(relocate_decorations(converted, rows, [name], polar_references=polar_references))
        except NoClearPlacement:
            skipped.append(name)
    return {"candidates": len(candidates), "moves": moved, "no_clear_placement": skipped}


def repair_south_guards(source, converted, rows, source_width, source_height, target_types, source_rows=None):
    """Restore guards placed inside an upright target after second-level rotation."""
    width, height = len(rows[0]), len(rows)
    original = {obj["instanceName"]: obj for obj in source}
    current = {obj["instanceName"]: obj for obj in converted}
    _, old_interactions, _ = occupants(source, source_width, source_height, False)
    old_pairs = {tuple(sorted((first[0], second[0]))) for _, first, second in old_interactions}
    prior_interactive = {item[0] for _, first, second in old_interactions for item in (first, second)}
    _, interactions = blocking_pairs(converted, width, height)

    def cells(obj):
        mask = obj["template"]["mask"]
        return [((obj["x"] - len(line) + 1 + col) % width,
                 (obj["y"] - len(mask) + 1 + row) % height, kind)
                for row, line in enumerate(mask) for col, kind in enumerate(line) if kind in "BHAT"]

    def entrance(obj, wrap):
        mask = obj["template"]["mask"]
        visits = [(obj["x"] - len(line) + 1 + col, obj["y"] - len(mask) + 1 + row)
                  for row, line in enumerate(mask) for col, kind in enumerate(line) if kind in "AT"]
        if len(visits) != 1:
            return None
        x, y = visits[0]
        return (x % width, y % height) if wrap else (x, y)

    candidates = collections.defaultdict(set)
    for a, b in set(interactions) - old_pairs:
        for mine, guard in ((a, b), (b, a)):
            m, g = original[mine], original[guard]
            if m["type"] not in target_types or not (g["type"] == "monster" or g["type"].startswith("randomMonster")):
                continue
            if m["l"] != 1 or g["l"] != 1 or guard in prior_interactive:
                continue
            old_mine, old_guard = entrance(m, False), entrance(g, False)
            if old_mine and old_guard == (old_mine[0], old_mine[1] + 1):
                candidates[guard].add(mine)

    occupied = collections.defaultdict(set)
    for obj in converted:
        for x, y, _ in cells(obj):
            occupied[x, y].add(obj["instanceName"])
    moves, skipped = [], []
    for guard, mines in sorted(candidates.items()):
        g = current[guard]
        if len(mines) != 1 or len(cells(g)) != 1 or entrance(g, True) is None:
            skipped.append(guard)
            continue
        mine = next(iter(mines))
        mine_pos = entrance(current[mine], True)
        if mine_pos is None:
            skipped.append(guard)
            continue
        target = (mine_pos[0], (mine_pos[1] + 1) % height)
        terrain = rows[target[1]][target[0]]
        parts = list(CELL_PART.finditer(terrain))
        overlays_ok = len(parts) == 1
        source_pos = entrance(original[guard], False)
        if source_rows is not None and source_pos and 0 <= source_pos[0] < source_width and 0 <= source_pos[1] < source_height:
            source_parts = list(CELL_PART.finditer(source_rows[source_pos[1]][source_pos[0]]))
            overlays_ok = (len(parts) > 1 and len(parts) == len(source_parts)
                           and all(part.group(1).startswith("p") for part in parts[1:])
                           and [part.group(1) for part in parts] == [part.group(1) for part in source_parts]) or overlays_ok
        if occupied[target] - {guard} or terrain[:2] in ("wt", "rc") or not overlays_ok:
            skipped.append(guard)
            continue
        before = entrance(g, True)
        occupied[before].discard(guard)
        g["x"] = (g["x"] + target[0] - before[0]) % width
        g["y"] = (g["y"] + target[1] - before[1]) % height
        occupied[target].add(guard)
        moves.append({"guard": guard, "object": mine, "from": before, "to": target})
    return {"candidates": len(candidates), "moves": moves, "skipped": skipped}


def repair_mine_guards(source, converted, rows, source_width, source_height, source_rows=None):
    return repair_south_guards(source, converted, rows, source_width, source_height, {"mine"}, source_rows)


def source_gate_pairs(objects, width, height, toroidal):
    """Reproduce the engine's nondecorative, two-level gate pairing order."""
    gates = []
    for obj in objects:
        if obj["type"] != "subterraneanGate":
            continue
        mask = obj["template"]["mask"]
        entrances = [(obj["x"] - (len(line) - 1 - col), obj["y"] - (len(mask) - 1 - row), obj["l"])
                     for row, line in enumerate(mask) for col, kind in enumerate(line) if kind in "AT"]
        if not entrances:  # the engine gives decorative gates separate channels
            continue
        if len(entrances) != 1:
            raise ValueError("Cannot infer pairing for a gate with multiple entrances")
        gates.append((obj["instanceName"], entrances[0]))
    if len({position for _, position in gates}) != len(gates):
        raise ValueError("Cannot infer deterministic pairing for gates at the same position")
    gates.sort(key=lambda item: (item[1][2], item[1][1], item[1][0]))
    paired, used = set(), set()
    for name, (x, y, z) in gates:
        if name in used:
            continue
        candidates = []
        for other, (ox, oy, oz) in gates:
            if other == name or other in used or oz <= z:
                continue
            dx, dy = abs(x - ox), abs(y - oy)
            if toroidal:
                dx, dy = min(dx % width, (-dx) % width), min(dy % height, (-dy) % height)
            candidates.append((dx * dx + dy * dy, other))
        if candidates:
            # min is stable on equal distances because gates are already in engine order.
            best = min(candidates, key=lambda item: item[0])[1]
            paired.add(tuple(sorted((name, best))))
            used.update((name, best))
    return paired


def replace_gate_pairs(source, converted, pairs, width, height, toroidal):
    """Turn explicit, original gate pairs into isolated two-way portal pairs."""
    if not pairs:
        return []
    old_by_name = {obj["instanceName"]: obj for obj in source}
    new_by_name = {obj["instanceName"]: obj for obj in converted}
    original_pairs = source_gate_pairs(source, width, height, toroidal)
    occupied_subtypes = {obj["subtype"].removeprefix("core:") for obj in source if obj["type"] == "monolithTwoWay"}
    used_names = set()
    report = []
    for first, second, subtype in pairs:
        if subtype not in {f"monolith{i}" for i in range(1, 9)} or subtype in occupied_subtypes:
            raise ValueError(f"Two-way monolith subtype is unavailable: {subtype!r}")
        if first == second or first in used_names or second in used_names:
            raise ValueError("Each gate must appear in at most one pair")
        if first not in old_by_name or second not in old_by_name:
            raise ValueError("Gate pair references an unknown object")
        if tuple(sorted((first, second))) not in original_pairs:
            raise ValueError("Gate pair does not match source-map pairing")
        for name in (first, second):
            if old_by_name[name].get("options"):
                raise ValueError("Gate has custom options requiring manual handling")
            new_by_name[name]["type"] = "monolithTwoWay"
            new_by_name[name]["subtype"] = subtype
        used_names.update((first, second))
        occupied_subtypes.add(subtype)
        report.append({"gates": [first, second], "monolith_subtype": subtype})
    return report


def nested_positions(value, path=()):
    if isinstance(value, dict):
        if {"x", "y", "l"} <= value.keys():
            yield path, value
        for key, item in value.items():
            yield from nested_positions(item, path + (key,))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            yield from nested_positions(item, path + (index,))


def assemble(header, surface, underground, objects, fragment_seed=None, polar_seed=None,
             oceanize=False, snow_island=None, gate_pairs=(), relocate=(), repair_mountains=False,
             required_mods=(), repair_guards=False, repair_other_guards=False, relocate_obstacles=(),
             reserve_tiles=()):
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
    replaced_gates = replace_gate_pairs(objects, converted, gate_pairs, width, height,
                                       header.get("topology") == "toroidal")
    rows = [[rotate_cell_art(cell) for cell in reversed(row)] for row in reversed(underground)]
    rows += copy.deepcopy(surface)
    polar = None
    if polar_seed is not None:
        component = snow_component(rows, polar_seed)
        if any(y >= height for _, y in component):
            raise ValueError("Polar component must lie within the rotated second level")
        if any(len(list(CELL_PART.finditer(rows[y][x]))) != 1 for x, y in component):
            raise ValueError("Polar component has road or river overlays requiring manual handling")
        references = polar_object_references(objects, component, width, height)
        if moved.intersection(references):
            raise ValueError("An object intersects both the east fragment and polar component")
        rows, north_depth, snow_cells = polarize_snow_island(rows, polar_seed)
        for index, (source, result) in enumerate(zip(objects, converted)):
            if source["instanceName"] in references:
                converted[index] = place_polar_object(source, references[source["instanceName"]], component,
                                                      width, height, len(rows))
            else:
                result["y"] = (result["y"] + north_depth) % len(rows)
        polar = {"source_snow_cells": snow_cells, "north_band_rows": north_depth,
                 "south_band_rows": len(rows) - 2 * height - north_depth,
                 "objects_moved": len(references)}
    rock = None
    if oceanize:
        ocean_rows, rock_count = oceanize_edge_rock(rows)
        affected_cells = changed_cells(rows, ocean_rows)
        if any(len(list(CELL_PART.finditer(rows[y][x]))) != 1 for x, y in affected_cells):
            raise ValueError("Boundary rock has road or river overlays requiring manual handling")
        affected_objects, _ = active_objects_on_cells(converted, affected_cells, width, len(rows))
        interactive = {obj["instanceName"] for obj in converted if obj["instanceName"] in affected_objects
                       and any(kind in "AT" for line in obj["template"]["mask"] for kind in line)}
        if interactive:
            raise ValueError("Boundary rock contains visitable objects requiring manual handling")
        rock = {"cells": rock_count, "affected_nonvisitable_objects": sorted(affected_objects)}
        rows = ocean_rows
    greenland = None
    if snow_island is not None:
        island_rows, island_count = add_illustrative_snow_island(rows, snow_island)
        affected_cells = changed_cells(rows, island_rows)
        affected_objects, _ = active_objects_on_cells(converted, affected_cells, width, len(rows))
        if affected_objects:
            raise ValueError("Illustrative snow island overlaps objects requiring manual handling")
        greenland = {"cells": island_count, "center": list(snow_island), "objects": 0}
        rows = island_rows
    relocated = relocate_decorations(converted, rows, relocate, polar_references=references if polar else None)
    mountain_repair = (repair_mountain_interactions(objects, converted, rows, width, height,
                                                   references if polar else None)
                       if repair_mountains else None)
    if relocate_obstacles:
        _, original_interactions, _ = occupants(objects, width, height, False)
        prior_interactive = {item[0] for _, first, second in original_interactions for item in (first, second)}
        if any(name in prior_interactive for name in relocate_obstacles):
            raise ValueError("Selected obstacle has a source interactive overlap")
    reserved = {tuple(tile) for tile in reserve_tiles}
    if any(len(tile) != 2 or not (0 <= tile[0] < width and 0 <= tile[1] < len(rows)) for tile in reserved):
        raise ValueError("Reserved tile must be inside the destination map")
    obstacles = relocate_decorations(converted, rows, relocate_obstacles,
                                    polar_references=references if polar else None,
                                    allowed_types={"mountain", "spruces"}, reserved=reserved)
    guard_repair = repair_mine_guards(objects, converted, rows, width, height, underground) if repair_guards else None
    other_guards = (repair_south_guards(objects, converted, rows, width, height,
                    {"randomTown", "arena", "creatureGeneratorCommon", "university", "waterWheel",
                     "libraryOfEnlightenment", "schoolOfWar", "colosseumOfTheMagi", "abandonedMine"}, underground)
                    if repair_other_guards else None)
    if polar is not None:
        for obj in converted:
            reference = references.get(obj["instanceName"])
            if reference is not None:
                _, _, dx, dy, _ = reference
                if rows[(obj["y"] - dy) % len(rows)][(obj["x"] - dx) % width][:2] != "sn":
                    raise ValueError(f"Polar object reference is not on snow: {obj['instanceName']!r}")
        water_cells = {(x, y) for y, row in enumerate(rows) for x, cell in enumerate(row) if cell[:2] == "wt"}
        polar_objects = [obj for obj in converted if obj["instanceName"] in references]
        polar_on_water, _ = active_objects_on_cells(polar_objects, water_cells, width, len(rows))
        polar["objects_with_active_cells_on_water"] = sorted(polar_on_water)
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

    new_header["mapLevels"] = {"surface": {"height": len(rows), "index": 0,
                                           "layer": "core:surface", "width": width}}
    new_header["versionMajor"] = TAGGED_TOROIDAL_VERSION
    new_header["topology"] = "toroidal"
    if required_mods:
        mods = new_header.get("mods")
        if mods is None:
            mods = {"allOf": []}
        if not isinstance(mods, dict) or not isinstance(mods.get("allOf"), list):
            raise ValueError("Cannot append unversioned mod requirements to this header format")
        if any(not isinstance(mod, str) or not mod or mod != mod.strip() for mod in mods["allOf"]):
            raise ValueError("Invalid existing mod requirement")
        for mod in required_mods:
            if not isinstance(mod, str) or not mod or mod != mod.strip():
                raise ValueError("Invalid mod requirement")
            if mod not in mods["allOf"]:
                mods["allOf"].append(mod)
        new_header["mods"] = mods
    original, original_interactions, clipped = occupants(objects, width, height, False)
    original_interaction_pairs = {tuple(sorted((first[0], second[0])))
                                  for _, first, second in original_interactions}
    assembled, interactions = blocking_pairs(converted, width, len(rows))
    new = assembled - original
    return new_header, rows, converted, {
        "dimensions": [width, len(rows)], "objects": len(converted),
        "source_outside_anchors": sum(not (0 <= obj["x"] < width and 0 <= obj["y"] < height)
                                      for obj in objects),
        "source_clipped_blocking_cells": clipped,
        "new_blocking_pairs_estimate": len(new),
        "new_interaction_occurrences_estimate": sum(pair not in original_interaction_pairs for pair in interactions),
        "east_fragment": fragment,
        "east_fragment_objects_moved": len(moved),
        "polar_component": polar,
        "oceanized_boundary_rock": rock,
        "illustrative_snow_island": greenland,
        "subterranean_gates": sum(obj["type"] == "subterraneanGate" for obj in converted),
        "replacement_gate_pairs": replaced_gates,
        "relocated_decorations": relocated,
        "relocated_obstacles": obstacles,
        "reserved_obstacle_tiles": sorted(reserved),
        "automatic_mountain_repair": mountain_repair,
        "mine_guard_repair": guard_repair,
        "other_guard_repair": other_guards,
        "limitations": ["diagnostic draft only", "object and interaction overlaps unresolved",
                        "road artwork and movement costs unverified",
                        "required mods not assessed"]
                       + (["polar geography not adapted"] if polar is None
                          else ["polar terrain artwork, object placement and collisions unverified"])
                       + (["east fragment not adapted"] if fragment is None
                          else ["east fragment object placement and overlaps unverified"])
                       + (["oceanized rock and affected decorations unverified"] if rock is not None else [])
                       + (["new snow island is schematic and has no objects"] if greenland is not None else [])
                       + (["remaining subterranean gates are unpaired"]
                          if any(obj["type"] == "subterraneanGate" for obj in converted) else [])
                       + (["replacement monolith behavior differs from subterranean gates"] if replaced_gates else [])
                       + (["automatic mountain moves may alter routes and need review"] if mountain_repair else [])
                       + (["selected obstacle moves may alter routes and need review"] if obstacles else [])
                       + (["guard moves may change nearby guard coverage"] if guard_repair or other_guards else []),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source")
    parser.add_argument("output", help="New draft VMAP; existing files are not overwritten")
    parser.add_argument("--rejoin-east-fragment", nargs=2, type=int, metavar=("X", "Y"),
                        help="Move one second-level east-edge land component and its objects to the surface")
    parser.add_argument("--polar-seed", nargs=2, type=int, metavar=("X", "Y"),
                        help="Move a snow component and its objects around new north/south bands")
    parser.add_argument("--oceanize-edge-rock", action="store_true",
                        help="Replace boundary-connected rock with ocean where no visitable objects are affected")
    parser.add_argument("--illustrative-snow-island", nargs=2, type=int, metavar=("X", "Y"),
                        help="Add a schematic unpopulated snow island in clear water")
    parser.add_argument("--gate-pair", nargs=3, action="append", metavar=("GATE1", "GATE2", "SUBTYPE"),
                        help="Replace a verified original gate pair with an unused two-way monolith subtype")
    parser.add_argument("--relocate-mountain", action="append", default=[], metavar="INSTANCE",
                        help="Move a selected plain mountain to nearby unoccupied dry terrain")
    parser.add_argument("--repair-mountain-interactions", action="store_true",
                        help="Try to move mountains that newly overlap interactive cells")
    parser.add_argument("--require-mod", action="append", default=[], metavar="MOD_ID",
                        help="Declare an unversioned required mod; does not discover dependencies")
    parser.add_argument("--repair-mine-guards", action="store_true",
                        help="Restore eligible mine guards to their original relative entrance position")
    parser.add_argument("--repair-other-guards", action="store_true",
                        help="Restore eligible guards of selected non-mine object types")
    parser.add_argument("--relocate-obstacle", action="append", default=[], metavar="INSTANCE",
                        help="Move an explicitly selected plain mountain or spruce with no source interactive overlap")
    parser.add_argument("--reserve-tile", action="append", default=[], nargs=2, type=int, metavar=("X", "Y"),
                        help="Keep this destination cell free during selected obstacle relocation")
    args = parser.parse_args()
    with zipfile.ZipFile(args.source) as archive:
        expected = {"header.json", "surface_terrain.json", "underground_terrain.json", "objects.json"}
        if not expected.issubset(archive.namelist()):
            raise ValueError("Missing source VMAP members")
        extras = {name: archive.read(name) for name in archive.namelist() if name not in expected}
        header, rows, objects, report = assemble(
            load_json(archive, "header.json"), load_json(archive, "surface_terrain.json"),
            load_json(archive, "underground_terrain.json"), load_json(archive, "objects.json"),
            args.rejoin_east_fragment, args.polar_seed, args.oceanize_edge_rock,
            args.illustrative_snow_island, args.gate_pair or (), args.relocate_mountain,
            args.repair_mountain_interactions, args.require_mod, args.repair_mine_guards, args.repair_other_guards,
            args.relocate_obstacle, args.reserve_tile)
    with zipfile.ZipFile(args.output, "x", compression=zipfile.ZIP_DEFLATED) as output:
        for name, data in extras.items():
            output.writestr(name, data)
        for name, value in (("header.json", header), ("surface_terrain.json", rows), ("objects.json", objects)):
            output.writestr(name, json.dumps(value, ensure_ascii=False, separators=(",", ":")))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

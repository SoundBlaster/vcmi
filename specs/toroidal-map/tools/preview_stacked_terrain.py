#!/usr/bin/env python3
"""Render an illustrative two-level VMAP layout as a terrain-only SVG.

The second level is rotated by 180 degrees and placed above the first.
This is not a playable map, an editor preview, or a seam-art check.
"""

import argparse
import collections
import json
import zipfile

from analyze_stacked_map import load_json


# Minimap unblocked colors from VCMI's core and HotA terrain definitions.
PALETTE = {
    "dt": "#523808", "sa": "#decf8c", "gr": "#004100",
    "sn": "#b5c7c6", "sw": "#4a866b", "rg": "#847131",
    "sb": "#843000", "lv": "#4a494a", "wt": "#085194",
    "rc": "#000000", "hl": "#54791a", "ws": "#b46927",
}


def stack_levels(surface, underground):
    if not surface or not underground or len(surface) != len(underground):
        raise ValueError("Expected two equally sized, nonempty levels")
    width = len(surface[0])
    if width == 0 or any(len(row) != width for row in surface + underground):
        raise ValueError("Expected rectangular levels with the same width")
    return [list(reversed(row)) for row in reversed(underground)] + surface


def east_land_fragment(underground, seed):
    """Find a planar land component touching only the second level's east edge."""
    if not underground or not underground[0] or any(len(row) != len(underground[0]) for row in underground):
        raise ValueError("Expected a nonempty rectangular second level")
    height, width = len(underground), len(underground[0])
    sx, sy = seed

    def is_land(x, y):
        code = underground[y][x][:2]
        if code not in PALETTE:
            raise ValueError(f"Unknown terrain code {code!r}")
        return code not in ("wt", "rc")

    if not (0 <= sx < width and 0 <= sy < height) or not is_land(sx, sy):
        raise ValueError("Fragment seed must identify a land tile on the second level")
    component, todo = {(sx, sy)}, [(sx, sy)]
    while todo:
        x, y = todo.pop()
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in component and is_land(nx, ny):
                component.add((nx, ny))
                todo.append((nx, ny))
    if not any(x == width - 1 for x, _ in component):
        raise ValueError("Fragment must touch the second level's east edge")
    if any(x == 0 or y == 0 or y == height - 1 for x, y in component):
        raise ValueError("Fragment must not touch other second-level boundaries")
    return component


def rejoin_east_land_fragment(surface, underground, seed):
    """Place a second-level east-edge land component on the surface preview.

    Keep its original orientation and preserve existing surface land on overlap.
    This is a terrain illustration, not a transfer of objects or map artwork.
    """
    stack_levels(surface, underground)  # validate compatible dimensions
    component = east_land_fragment(underground, seed)
    if any(surface[y][x][:2] == "rc" for x, y in component):
        raise ValueError("Fragment overlaps surface rock; manual review is required")

    new_surface = [list(row) for row in surface]
    new_underground = [list(row) for row in underground]
    copied, overlap = 0, 0
    for x, y in component:
        if surface[y][x][:2] == "wt":
            new_surface[y][x] = underground[y][x]
            copied += 1
        else:
            overlap += 1
        new_underground[y][x] = "wt0_"
    return new_surface, new_underground, {"source_cells": len(component), "copied": copied, "existing_land": overlap}


def render_svg(rows):
    width, height = len(rows[0]), len(rows)
    lines = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" '
             f'width="{width * 3}" height="{height * 3}" shape-rendering="crispEdges">']
    for y, row in enumerate(rows):
        start = 0
        while start < width:
            code = row[start][:2]
            if code not in PALETTE:
                raise ValueError(f"Unknown terrain code {code!r} at ({start},{y})")
            end = start + 1
            while end < width and row[end][:2] == code:
                end += 1
            lines.append(f'<rect x="{start}" y="{y}" width="{end - start}" height="1" '
                         f'fill="{PALETTE[code]}"/>')
            start = end
    lines.append("</svg>")
    return "\n".join(lines) + "\n"


def seam_summary(rows, join_index=None):
    def kind(tile):
        code = tile[:2]
        if code not in PALETTE:
            raise ValueError(f"Unknown terrain code {code!r}")
        return {"wt": "water", "rc": "rock"}.get(code, "land")

    def counts(pairs):
        result = collections.Counter(f"{kind(a)}/{kind(b)}" for a, b in pairs)
        return dict(sorted(result.items()))

    midpoint = len(rows) // 2 if join_index is None else join_index
    return {
        "west_east": counts((row[0], row[-1]) for row in rows),
        "north_south": counts(zip(rows[0], rows[-1])),
        "level_join": counts(zip(rows[midpoint - 1], rows[midpoint])),
    }


def snow_component(rows, seed):
    """Find a planar, 4-connected snow component that stays inside the preview."""
    width, height = len(rows[0]), len(rows)
    sx, sy = seed
    if not (0 <= sx < width and 0 <= sy < height) or rows[sy][sx][:2] != "sn":
        raise ValueError("Polar seed must identify a snow tile")
    component = {(sx, sy)}
    todo = [(sx, sy)]
    while todo:
        x, y = todo.pop()
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nx, ny = x + dx, y + dy
            if (0 <= nx < width and 0 <= ny < height and (nx, ny) not in component
                    and rows[ny][nx][:2] == "sn"):
                component.add((nx, ny))
                todo.append((nx, ny))
    if any(x == 0 or y == 0 or x == width - 1 or y == height - 1 for x, y in component):
        raise ValueError("Polar snow component must not touch the preview boundary")
    return component


def polarize_snow_island(rows, seed):
    """Move one isolated snow component to new bands around the vertical seam.

    This operates only on a terrain-color preview. It does not move objects,
    roads, rivers, events or terrain artwork and is not an authoring operation.
    """
    width = len(rows[0])
    component = snow_component(rows, seed)
    min_x = min(x for x, _ in component)
    max_x = max(x for x, _ in component)
    min_y = min(y for _, y in component)
    max_y = max(y for _, y in component)
    island_width = max_x - min_x + 1
    island_height = max_y - min_y + 1
    south_depth = island_height // 2
    north_depth = island_height - south_depth

    # New bands avoid overwriting the existing land. This increases the map
    # height in the illustration and does not decide its final dimensions.
    water = "wt0_"
    result = [[water] * width for _ in range(north_depth)]
    result += [[water if (x, y) in component else tile for x, tile in enumerate(row)]
               for y, row in enumerate(rows)]
    result += [[water] * width for _ in range(south_depth)]
    for dy in range(island_height):
        destination_y = (dy - south_depth) % len(result)
        for x in range(width):
            original_x = min_x + x * island_width // width
            if (original_x, min_y + dy) in component:
                result[destination_y][x] = rows[min_y + dy][original_x]
    return result, north_depth, len(component)


def oceanize_edge_rock(rows):
    """Replace boundary-connected rock in a terrain-only illustration."""
    width, height = len(rows[0]), len(rows)
    boundary = ([(x, 0) for x in range(width)] + [(x, height - 1) for x in range(width)]
                + [(0, y) for y in range(height)] + [(width - 1, y) for y in range(height)])
    component = {(x, y) for x, y in boundary if rows[y][x][:2] == "rc"}
    todo = list(component)
    while todo:
        x, y = todo.pop()
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nx, ny = x + dx, y + dy
            if (0 <= nx < width and 0 <= ny < height and (nx, ny) not in component
                    and rows[ny][nx][:2] == "rc"):
                component.add((nx, ny))
                todo.append((nx, ny))
    return [["wt0_" if (x, y) in component else tile for x, tile in enumerate(row)]
            for y, row in enumerate(rows)], len(component)


def add_illustrative_snow_island(rows, center):
    """Draw a schematic island in open water, only in the terrain preview."""
    width, height = len(rows[0]), len(rows)
    cx, cy = center
    cells = set()
    for dy in range(-18, 19):
        for dx in range(-10, 11):
            # A slightly tilted ellipse is an illustration, not original map data.
            shifted_x = dx - dy // 5
            if shifted_x * shifted_x * 18 * 18 + dy * dy * 7 * 7 <= 7 * 7 * 18 * 18:
                x, y = cx + dx, cy + dy
                if not (0 <= x < width and 0 <= y < height):
                    raise ValueError("Illustrative island would cross a preview boundary")
                if rows[y][x][:2] != "wt":
                    raise ValueError("Illustrative island requires open water")
                cells.add((x, y))
    result = [list(row) for row in rows]
    for x, y in cells:
        result[y][x] = "sn0_"
    return result, len(cells)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vmap")
    parser.add_argument("output", help="New SVG file; existing files are not overwritten")
    parser.add_argument("--polar-seed", nargs=2, type=int, metavar=("X", "Y"),
                        help="Move an isolated snow component to new polar bands in the preview")
    parser.add_argument("--rejoin-east-fragment", nargs=2, type=int, metavar=("X", "Y"),
                        help="Move an east-edge second-level land component onto the surface preview")
    parser.add_argument("--oceanize-edge-rock", action="store_true",
                        help="Show boundary-connected rock as water in the preview")
    parser.add_argument("--illustrative-snow-island", nargs=2, type=int, metavar=("X", "Y"),
                        help="Draw a new schematic snow island in open water in the preview")
    args = parser.parse_args()
    with zipfile.ZipFile(args.vmap) as archive:
        surface = load_json(archive, "surface_terrain.json")
        underground = load_json(archive, "underground_terrain.json")
    if args.rejoin_east_fragment is not None:
        surface, underground, fragment = rejoin_east_land_fragment(surface, underground, args.rejoin_east_fragment)
        print("East fragment:", json.dumps(fragment))
    rows = stack_levels(surface, underground)
    join_index = len(underground)
    if args.polar_seed is not None:
        rows, north_depth, cells = polarize_snow_island(rows, args.polar_seed)
        join_index += north_depth
        print(f"Polar component: {cells} snow cells; preview dimensions {len(rows[0])}x{len(rows)}")
    if args.oceanize_edge_rock:
        rows, rock_cells = oceanize_edge_rock(rows)
        print(f"Boundary rock shown as ocean: {rock_cells} cells")
    if args.illustrative_snow_island is not None:
        rows, island_cells = add_illustrative_snow_island(rows, args.illustrative_snow_island)
        print(f"Illustrative snow island: {island_cells} new cells")
    svg = render_svg(rows)
    with open(args.output, "x", encoding="utf-8") as output:
        output.write(svg)
    print(f"Wrote terrain-only preview to {args.output}")
    print(json.dumps(seam_summary(rows, join_index), indent=2))


if __name__ == "__main__":
    main()

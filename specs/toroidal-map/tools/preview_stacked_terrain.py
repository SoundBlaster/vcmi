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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vmap")
    parser.add_argument("output", help="New SVG file; existing files are not overwritten")
    parser.add_argument("--polar-seed", nargs=2, type=int, metavar=("X", "Y"),
                        help="Move an isolated snow component to new polar bands in the preview")
    args = parser.parse_args()
    with zipfile.ZipFile(args.vmap) as archive:
        surface = load_json(archive, "surface_terrain.json")
        underground = load_json(archive, "underground_terrain.json")
    rows = stack_levels(surface, underground)
    join_index = len(underground)
    if args.polar_seed is not None:
        rows, north_depth, cells = polarize_snow_island(rows, args.polar_seed)
        join_index += north_depth
        print(f"Polar component: {cells} snow cells; preview dimensions {len(rows[0])}x{len(rows)}")
    svg = render_svg(rows)
    with open(args.output, "x", encoding="utf-8") as output:
        output.write(svg)
    print(f"Wrote terrain-only preview to {args.output}")
    print(json.dumps(seam_summary(rows, join_index), indent=2))


if __name__ == "__main__":
    main()

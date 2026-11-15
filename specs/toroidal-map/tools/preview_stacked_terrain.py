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


def seam_summary(rows):
    def kind(tile):
        code = tile[:2]
        if code not in PALETTE:
            raise ValueError(f"Unknown terrain code {code!r}")
        return {"wt": "water", "rc": "rock"}.get(code, "land")

    def counts(pairs):
        result = collections.Counter(f"{kind(a)}/{kind(b)}" for a, b in pairs)
        return dict(sorted(result.items()))

    midpoint = len(rows) // 2
    return {
        "west_east": counts((row[0], row[-1]) for row in rows),
        "north_south": counts(zip(rows[0], rows[-1])),
        "level_join": counts(zip(rows[midpoint - 1], rows[midpoint])),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vmap")
    parser.add_argument("output", help="New SVG file; existing files are not overwritten")
    args = parser.parse_args()
    with zipfile.ZipFile(args.vmap) as archive:
        surface = load_json(archive, "surface_terrain.json")
        underground = load_json(archive, "underground_terrain.json")
    rows = stack_levels(surface, underground)
    svg = render_svg(rows)
    with open(args.output, "x", encoding="utf-8") as output:
        output.write(svg)
    print(f"Wrote terrain-only preview to {args.output}")
    print(json.dumps(seam_summary(rows), indent=2))


if __name__ == "__main__":
    main()
